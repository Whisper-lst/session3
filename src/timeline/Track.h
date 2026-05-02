#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>

#include "Types.h"
#include "Clip.h"

namespace ve {

enum class TrackType {
    Video,
    Audio,
    Text
};

class Track {
public:
    Track();
    Track(TrackType type, const std::string& name = "");
    ~Track();
    
    int getId() const { return m_id; }
    void setId(int id) { m_id = id; }
    
    TrackType getType() const { return m_type; }
    void setType(TrackType type) { m_type = type; }
    
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    bool isMuted() const { return m_muted; }
    void setMuted(bool muted) { m_muted = muted; }
    
    bool isSolo() const { return m_solo; }
    void setSolo(bool solo) { m_solo = solo; }
    
    bool isLocked() const { return m_locked; }
    void setLocked(bool locked) { m_locked = locked; }
    
    float getVolume() const { return m_volume; }
    void setVolume(float volume) { m_volume = volume; }
    
    float getOpacity() const { return m_opacity; }
    void setOpacity(float opacity) { m_opacity = opacity; }
    
    bool addClip(const Clip& clip);
    bool removeClip(int clipId);
    Clip* getClip(int clipId);
    const Clip* getClip(int clipId) const;
    
    std::vector<Clip*> getClipsAtTime(TimePoint time);
    std::vector<const Clip*> getClipsAtTime(TimePoint time) const;
    
    std::vector<Clip*> getClips();
    const std::map<int, std::unique_ptr<Clip>>& getClipMap() const { return m_clips; }
    
    Duration getDuration() const;
    
    bool hasClipOverlap(const Clip& clip) const;
    std::vector<Clip*> getOverlappingClips(const Clip& clip);
    
    void sortClips();
    
    void clear();
    bool isEmpty() const { return m_clips.empty(); }
    
    int getClipCount() const { return static_cast<int>(m_clips.size()); }

private:
    int m_id = 0;
    TrackType m_type = TrackType::Video;
    std::string m_name;
    
    bool m_muted = false;
    bool m_solo = false;
    bool m_locked = false;
    
    float m_volume = 1.0f;
    float m_opacity = 1.0f;
    
    std::map<int, std::unique_ptr<Clip>> m_clips;
    mutable std::mutex m_mutex;
};

}
