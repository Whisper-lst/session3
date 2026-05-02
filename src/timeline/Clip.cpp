#include "Clip.h"
#include "Logger.h"

namespace ve {

Clip::Clip() {
    LOG_DEBUG("Clip created");
}

Clip::Clip(const std::string& sourceFile, ClipType type)
    : m_sourceFile(sourceFile), m_type(type) {
    LOG_DEBUG("Clip created: %s", sourceFile.c_str());
}

Clip::~Clip() {
    LOG_DEBUG("Clip destroyed");
}

bool Clip::removeEffect(int effectId) {
    for (auto it = m_effects.begin(); it != m_effects.end(); ++it) {
        if (*it == effectId) {
            m_effects.erase(it);
            return true;
        }
    }
    return false;
}

bool Clip::containsTime(TimePoint timelineTime) const {
    return timelineTime >= m_timelinePosition && 
           timelineTime < m_timelinePosition + getDuration();
}

TimePoint Clip::getSourceTime(TimePoint timelineTime) const {
    if (!containsTime(timelineTime)) {
        return TimePoint(-1);
    }
    
    Duration offset = timelineTime - m_timelinePosition;
    if (m_speed != 1.0) {
        offset = Duration(static_cast<int64_t>(offset.count() / m_speed));
    }
    
    return m_sourceIn + offset;
}

Clip Clip::clone() const {
    Clip clone;
    clone.m_sourceFile = m_sourceFile;
    clone.m_name = m_name;
    clone.m_type = m_type;
    clone.m_timelinePosition = m_timelinePosition;
    clone.m_sourceIn = m_sourceIn;
    clone.m_sourceOut = m_sourceOut;
    clone.m_speed = m_speed;
    clone.m_opacity = m_opacity;
    clone.m_volume = m_volume;
    clone.m_muted = m_muted;
    clone.m_locked = m_locked;
    clone.m_effects = m_effects;
    return clone;
}

}
