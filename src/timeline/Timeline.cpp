#include "Timeline.h"
#include "Logger.h"
#include "VideoDecoder.h"

#include <algorithm>
#include <cstring>

namespace ve {

Timeline::Timeline() {
    LOG_DEBUG("Timeline created");
}

Timeline::~Timeline() {
    LOG_DEBUG("Timeline destroyed");
}

int Timeline::addTrack(TrackType type, const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    int trackId = m_nextTrackId++;
    auto track = std::make_unique<Track>(type, name.empty() ? ("Track " + std::to_string(trackId)) : name);
    track->setId(trackId);
    
    m_tracks[trackId] = std::move(track);
    m_trackTypes[trackId] = type;
    
    LOG_INFO("Added track %d: %s", trackId, name.c_str());
    return trackId;
}

bool Timeline::removeTrack(int trackId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_tracks.find(trackId);
    if (it != m_tracks.end()) {
        m_tracks.erase(it);
        m_trackTypes.erase(trackId);
        LOG_INFO("Removed track %d", trackId);
        return true;
    }
    return false;
}

Track* Timeline::getTrack(int trackId) {
    auto it = m_tracks.find(trackId);
    if (it != m_tracks.end()) {
        return it->second.get();
    }
    return nullptr;
}

const Track* Timeline::getTrack(int trackId) const {
    auto it = m_tracks.find(trackId);
    if (it != m_tracks.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<int> Timeline::getTrackIds() const {
    std::vector<int> result;
    for (const auto& pair : m_tracks) {
        result.push_back(pair.first);
    }
    return result;
}

std::vector<Track*> Timeline::getTracks() {
    std::vector<Track*> result;
    for (const auto& pair : m_tracks) {
        result.push_back(pair.second.get());
    }
    return result;
}

Duration Timeline::getDuration() const {
    Duration maxDuration = m_duration;
    for (const auto& pair : m_tracks) {
        Duration trackDuration = pair.second->getDuration();
        if (trackDuration > maxDuration) {
            maxDuration = trackDuration;
        }
    }
    return maxDuration;
}

void Timeline::setCurrentTime(TimePoint time) {
    if (time < TimePoint(0)) {
        time = TimePoint(0);
    }
    Duration duration = getDuration();
    if (time > duration) {
        time = duration;
    }
    m_currentTime = time;
}

bool Timeline::insertClip(int trackId, const Clip& clip) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    Track* track = getTrack(trackId);
    if (!track) {
        LOG_ERROR("Track %d not found", trackId);
        return false;
    }
    
    return track->addClip(clip);
}

bool Timeline::removeClip(int trackId, int clipId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    Track* track = getTrack(trackId);
    if (!track) {
        return false;
    }
    
    return track->removeClip(clipId);
}

Clip* Timeline::getClip(int trackId, int clipId) {
    Track* track = getTrack(trackId);
    if (!track) {
        return nullptr;
    }
    return track->getClip(clipId);
}

const Clip* Timeline::getClip(int trackId, int clipId) const {
    const Track* track = getTrack(trackId);
    if (!track) {
        return nullptr;
    }
    return track->getClip(clipId);
}

bool Timeline::moveClip(int trackId, int clipId, TimePoint newPosition) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    Track* track = getTrack(trackId);
    if (!track) {
        return false;
    }
    
    Clip* clip = track->getClip(clipId);
    if (!clip) {
        return false;
    }
    
    if (track->isLocked()) {
        LOG_WARNING("Cannot move clip on locked track");
        return false;
    }
    
    TimePoint oldPosition = clip->getTimelinePosition();
    
    if (oldPosition == newPosition) {
        return true;
    }
    
    Clip tempClip = *clip;
    tempClip.setTimelinePosition(newPosition);
    
    if (track->hasClipOverlap(tempClip)) {
        LOG_WARNING("Cannot move clip - would overlap with other clips");
        return false;
    }
    
    clip->setTimelinePosition(newPosition);
    return true;
}

bool Timeline::trimClipStart(int trackId, int clipId, Duration offset) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    Track* track = getTrack(trackId);
    if (!track) {
        return false;
    }
    
    Clip* clip = track->getClip(clipId);
    if (!clip) {
        return false;
    }
    
    if (track->isLocked() || clip->isLocked()) {
        LOG_WARNING("Cannot trim locked clip/track");
        return false;
    }
    
    Duration sourceIn = clip->getSourceIn() + offset;
    Duration duration = clip->getDuration() - offset;
    
    if (duration <= Duration(0)) {
        LOG_WARNING("Invalid trim operation");
        return false;
    }
    
    clip->setSourceIn(sourceIn);
    clip->setTimelinePosition(clip->getTimelinePosition() + offset);
    
    return true;
}

bool Timeline::trimClipEnd(int trackId, int clipId, Duration offset) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    Track* track = getTrack(trackId);
    if (!track) {
        return false;
    }
    
    Clip* clip = track->getClip(clipId);
    if (!clip) {
        return false;
    }
    
    if (track->isLocked() || clip->isLocked()) {
        LOG_WARNING("Cannot trim locked clip/track");
        return false;
    }
    
    Duration sourceOut = clip->getSourceOut() - offset;
    Duration duration = clip->getDuration() - offset;
    
    if (duration <= Duration(0)) {
        LOG_WARNING("Invalid trim operation");
        return false;
    }
    
    clip->setSourceOut(sourceOut);
    
    return true;
}

bool Timeline::splitClip(int trackId, int clipId, TimePoint splitTime) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    Track* track = getTrack(trackId);
    if (!track) {
        return false;
    }
    
    Clip* originalClip = track->getClip(clipId);
    if (!originalClip) {
        return false;
    }
    
    if (!originalClip->containsTime(splitTime)) {
        LOG_WARNING("Split time is not within clip range");
        return false;
    }
    
    Duration splitOffset = splitTime - originalClip->getTimelinePosition();
    Duration originalSourceIn = originalClip->getSourceIn();
    Duration originalSourceOut = originalClip->getSourceOut();
    
    Clip firstClip = originalClip->clone();
    firstClip.setId(getNextClipId());
    firstClip.setSourceOut(originalSourceIn + splitOffset);
    
    Clip secondClip = originalClip->clone();
    secondClip.setId(getNextClipId());
    secondClip.setSourceIn(originalSourceIn + splitOffset);
    secondClip.setTimelinePosition(splitTime);
    
    track->removeClip(clipId);
    track->addClip(firstClip);
    track->addClip(secondClip);
    
    LOG_INFO("Split clip %d at time %lld", clipId, splitTime.count());
    return true;
}

bool Timeline::getFrameAtTime(TimePoint time, VideoFrame& outFrame) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto clips = getTrackClipsAtTime(time);
    if (clips.empty()) {
        return false;
    }
    
    std::sort(clips.begin(), clips.end(), 
        [](const std::pair<Track*, Clip*>& a, const std::pair<Track*, Clip*>& b) {
            int idA = a.first->getId();
            int idB = b.first->getId();
            return idA > idB;
        });
    
    for (auto& pair : clips) {
        Track* track = pair.first;
        Clip* clip = pair.second;
        
        if (track->isMuted() || track->getType() != TrackType::Video) {
            continue;
        }
        
        if (clip->getType() != ClipType::Video) {
            continue;
        }
        
        const std::string& sourceFile = clip->getSourceFile();
        if (sourceFile.empty()) {
            continue;
        }
        
        VideoDecoder* decoder = getOrCreateDecoder(sourceFile);
        if (!decoder) {
            LOG_WARNING("Failed to get decoder for: %s", sourceFile.c_str());
            continue;
        }
        
        TimePoint sourceTime = clip->getSourceTime(time);
        
        if (clip->getSpeed() != 1.0) {
            Duration originalOffset = time - clip->getTimelinePosition();
            sourceTime = TimePoint(clip->getSourceIn().count() + 
                static_cast<int64_t>(originalOffset.count() / clip->getSpeed()));
        }
        
        VideoFrame tempFrame;
        if (decoder->decodeFrameAt(sourceTime, tempFrame)) {
            outFrame = tempFrame;
            outFrame.opacity = clip->getOpacity();
            return true;
        }
    }
    
    return false;
}

VideoDecoder* Timeline::getOrCreateDecoder(const std::string& sourceFile) {
    auto it = m_decoderCache.find(sourceFile);
    if (it != m_decoderCache.end()) {
        return it->second.get();
    }
    
    auto decoder = std::make_unique<VideoDecoder>();
    if (decoder->open(sourceFile)) {
        VideoDecoder* rawPtr = decoder.get();
        m_decoderCache[sourceFile] = std::move(decoder);
        LOG_INFO("Cached decoder for: %s", sourceFile.c_str());
        return rawPtr;
    }
    
    LOG_ERROR("Failed to open decoder for: %s", sourceFile.c_str());
    return nullptr;
}

void Timeline::clearDecoderCache() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_decoderCache.clear();
    LOG_INFO("Decoder cache cleared");
}

std::vector<Clip*> Timeline::getClipsAtTime(TimePoint time) {
    std::vector<Clip*> result;
    for (auto& pair : m_tracks) {
        auto clips = pair.second->getClipsAtTime(time);
        result.insert(result.end(), clips.begin(), clips.end());
    }
    return result;
}

std::vector<std::pair<Track*, Clip*>> Timeline::getTrackClipsAtTime(TimePoint time) {
    std::vector<std::pair<Track*, Clip*>> result;
    for (auto& pair : m_tracks) {
        Track* track = pair.second.get();
        auto clips = track->getClipsAtTime(time);
        for (Clip* clip : clips) {
            result.emplace_back(track, clip);
        }
    }
    return result;
}

void Timeline::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_tracks.clear();
    m_trackTypes.clear();
    m_duration = Duration(0);
    m_currentTime = TimePoint(0);
}

bool Timeline::isEmpty() const {
    return m_tracks.empty();
}

}
