#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>

#include "Types.h"

namespace ve {

enum class ClipType {
    Video,
    Audio,
    Image,
    Text
};

class Clip {
public:
    Clip();
    Clip(const std::string& sourceFile, ClipType type);
    ~Clip();
    
    int getId() const { return m_id; }
    void setId(int id) { m_id = id; }
    
    const std::string& getSourceFile() const { return m_sourceFile; }
    void setSourceFile(const std::string& file) { m_sourceFile = file; }
    
    ClipType getType() const { return m_type; }
    void setType(ClipType type) { m_type = type; }
    
    TimePoint getTimelinePosition() const { return m_timelinePosition; }
    void setTimelinePosition(TimePoint position) { m_timelinePosition = position; }
    
    Duration getSourceIn() const { return m_sourceIn; }
    void setSourceIn(Duration position) { m_sourceIn = position; }
    
    Duration getSourceOut() const { return m_sourceOut; }
    void setSourceOut(Duration position) { m_sourceOut = position; }
    
    Duration getDuration() const { return m_sourceOut - m_sourceIn; }
    
    TimePoint getTimelineStart() const { return m_timelinePosition; }
    TimePoint getTimelineEnd() const { return m_timelinePosition + getDuration(); }
    
    void setSpeed(double speed) { m_speed = speed; }
    double getSpeed() const { return m_speed; }
    
    void setOpacity(float opacity) { m_opacity = opacity; }
    float getOpacity() const { return m_opacity; }
    
    void setVolume(float volume) { m_volume = volume; }
    float getVolume() const { return m_volume; }
    
    void setMuted(bool muted) { m_muted = muted; }
    bool isMuted() const { return m_muted; }
    
    void setLocked(bool locked) { m_locked = locked; }
    bool isLocked() const { return m_locked; }
    
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    void addEffect(int effectId) { m_effects.push_back(effectId); }
    bool removeEffect(int effectId);
    const std::vector<int>& getEffects() const { return m_effects; }
    void clearEffects() { m_effects.clear(); }
    
    bool containsTime(TimePoint timelineTime) const;
    
    TimePoint getSourceTime(TimePoint timelineTime) const;
    
    Clip clone() const;

private:
    int m_id = 0;
    std::string m_sourceFile;
    std::string m_name;
    ClipType m_type = ClipType::Video;
    
    TimePoint m_timelinePosition{0};
    Duration m_sourceIn{0};
    Duration m_sourceOut{0};
    
    double m_speed = 1.0;
    float m_opacity = 1.0f;
    float m_volume = 1.0f;
    
    bool m_muted = false;
    bool m_locked = false;
    
    std::vector<int> m_effects;
};

}
