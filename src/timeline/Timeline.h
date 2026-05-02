#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <mutex>
#include <unordered_map>

#include "Types.h"
#include "Clip.h"
#include "Track.h"

namespace ve {

class VideoDecoder;

class Timeline {
public:
    Timeline();
    ~Timeline();
    
    void setName(const std::string& name) { m_name = name; }
    const std::string& getName() const { return m_name; }
    
    int addTrack(TrackType type, const std::string& name = "");
    bool removeTrack(int trackId);
    Track* getTrack(int trackId);
    const Track* getTrack(int trackId) const;
    
    std::vector<int> getTrackIds() const;
    std::vector<Track*> getTracks();
    
    Duration getDuration() const;
    void setDuration(Duration duration) { m_duration = duration; }
    
    TimePoint getCurrentTime() const { return m_currentTime; }
    void setCurrentTime(TimePoint time);
    
    bool insertClip(int trackId, const Clip& clip);
    bool removeClip(int trackId, int clipId);
    Clip* getClip(int trackId, int clipId);
    const Clip* getClip(int trackId, int clipId) const;
    
    bool moveClip(int trackId, int clipId, TimePoint newPosition);
    bool trimClipStart(int trackId, int clipId, Duration offset);
    bool trimClipEnd(int trackId, int clipId, Duration offset);
    bool splitClip(int trackId, int clipId, TimePoint splitTime);
    
    bool getFrameAtTime(TimePoint time, VideoFrame& outFrame);
    
    std::vector<Clip*> getClipsAtTime(TimePoint time);
    std::vector<std::pair<Track*, Clip*>> getTrackClipsAtTime(TimePoint time);
    
    void clear();
    bool isEmpty() const;
    
    int getNextTrackId() { return m_nextTrackId++; }
    int getNextClipId() { return m_nextClipId++; }
    
    int getTrackCount() const { return static_cast<int>(m_tracks.size()); }
    
    VideoDecoder* getOrCreateDecoder(const std::string& sourceFile);
    void clearDecoderCache();

private:
    std::string m_name;
    std::map<int, std::unique_ptr<Track>> m_tracks;
    std::map<int, TrackType> m_trackTypes;
    
    int m_nextTrackId = 1;
    int m_nextClipId = 1;
    
    Duration m_duration{0};
    TimePoint m_currentTime{0};
    
    mutable std::mutex m_mutex;
    
    std::unordered_map<std::string, std::unique_ptr<VideoDecoder>> m_decoderCache;
};

}
