#include "Track.h"
#include "Logger.h"

namespace ve {

Track::Track() {
    LOG_DEBUG("Track created");
}

Track::Track(TrackType type, const std::string& name)
    : m_type(type), m_name(name) {
    LOG_DEBUG("Track created: %s", name.c_str());
}

Track::~Track() {
    LOG_DEBUG("Track destroyed");
}

bool Track::addClip(const Clip& clip) {
    if (m_locked) {
        LOG_WARNING("Cannot add clip to locked track");
        return false;
    }
    
    if (hasClipOverlap(clip)) {
        LOG_WARNING("Clip overlaps with existing clips");
        return false;
    }
    
    m_clips[clip.getId()] = std::make_unique<Clip>(clip);
    return true;
}

bool Track::removeClip(int clipId) {
    if (m_locked) {
        LOG_WARNING("Cannot remove clip from locked track");
        return false;
    }
    
    auto it = m_clips.find(clipId);
    if (it != m_clips.end()) {
        m_clips.erase(it);
        return true;
    }
    return false;
}

Clip* Track::getClip(int clipId) {
    auto it = m_clips.find(clipId);
    if (it != m_clips.end()) {
        return it->second.get();
    }
    return nullptr;
}

const Clip* Track::getClip(int clipId) const {
    auto it = m_clips.find(clipId);
    if (it != m_clips.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<Clip*> Track::getClipsAtTime(TimePoint time) {
    std::vector<Clip*> result;
    for (auto& pair : m_clips) {
        if (pair.second->containsTime(time)) {
            result.push_back(pair.second.get());
        }
    }
    return result;
}

std::vector<const Clip*> Track::getClipsAtTime(TimePoint time) const {
    std::vector<const Clip*> result;
    for (const auto& pair : m_clips) {
        if (pair.second->containsTime(time)) {
            result.push_back(pair.second.get());
        }
    }
    return result;
}

std::vector<Clip*> Track::getClips() {
    std::vector<Clip*> result;
    for (auto& pair : m_clips) {
        result.push_back(pair.second.get());
    }
    return result;
}

Duration Track::getDuration() const {
    Duration maxDuration(0);
    for (const auto& pair : m_clips) {
        const Clip* clip = pair.second.get();
        if (clip->getTimelineEnd() > maxDuration) {
            maxDuration = clip->getTimelineEnd();
        }
    }
    return maxDuration;
}

bool Track::hasClipOverlap(const Clip& clip) const {
    for (const auto& pair : m_clips) {
        const Clip* existing = pair.second.get();
        if (existing->getId() == clip.getId()) {
            continue;
        }
        
        if (clip.getTimelineStart() < existing->getTimelineEnd() &&
            clip.getTimelineEnd() > existing->getTimelineStart()) {
            return true;
        }
    }
    return false;
}

std::vector<Clip*> Track::getOverlappingClips(const Clip& clip) {
    std::vector<Clip*> result;
    for (auto& pair : m_clips) {
        Clip* existing = pair.second.get();
        if (existing->getId() == clip.getId()) {
            continue;
        }
        
        if (clip.getTimelineStart() < existing->getTimelineEnd() &&
            clip.getTimelineEnd() > existing->getTimelineStart()) {
            result.push_back(existing);
        }
    }
    return result;
}

void Track::clear() {
    if (m_locked) {
        LOG_WARNING("Cannot clear locked track");
        return;
    }
    m_clips.clear();
}

void Track::sortClips() {
}

}
