#pragma once

#include <string>
#include <vector>

#include "Types.h"

namespace ve {

class TimelineCursor {
public:
    TimelineCursor();
    ~TimelineCursor();
    
    TimePoint getPosition() const { return m_position; }
    void setPosition(TimePoint position);
    
    TimePoint getMinPosition() const { return m_minPosition; }
    void setMinPosition(TimePoint position) { m_minPosition = position; }
    
    TimePoint getMaxPosition() const { return m_maxPosition; }
    void setMaxPosition(TimePoint position) { m_maxPosition = position; }
    
    bool isPlaying() const { return m_isPlaying; }
    void play();
    void pause();
    void stop();
    
    void setLoop(bool loop) { m_loop = loop; }
    bool isLooping() const { return m_loop; }
    
    void setPlaybackSpeed(double speed) { m_playbackSpeed = speed; }
    double getPlaybackSpeed() const { return m_playbackSpeed; }
    
    void setInPoint(TimePoint point) { m_inPoint = point; }
    TimePoint getInPoint() const { return m_inPoint; }
    
    void setOutPoint(TimePoint point) { m_outPoint = point; }
    TimePoint getOutPoint() const { return m_outPoint; }
    
    void stepForward(int frames = 1);
    void stepBackward(int frames = 1);
    
    void goToStart();
    void goToEnd();
    void goToInPoint();
    void goToOutPoint();
    
    void setSnapEnabled(bool enabled) { m_snapEnabled = enabled; }
    bool isSnapEnabled() const { return m_snapEnabled; }
    
    void setSnapPoints(const std::vector<TimePoint>& points) { m_snapPoints = points; }
    const std::vector<TimePoint>& getSnapPoints() const { return m_snapPoints; }
    
    void addSnapPoint(TimePoint point);
    void removeSnapPoint(TimePoint point);
    void clearSnapPoints();

private:
    TimePoint m_position{0};
    TimePoint m_minPosition{0};
    TimePoint m_maxPosition{0};
    
    TimePoint m_inPoint{0};
    TimePoint m_outPoint{0};
    
    bool m_isPlaying = false;
    bool m_loop = false;
    double m_playbackSpeed = 1.0;
    
    bool m_snapEnabled = false;
    std::vector<TimePoint> m_snapPoints;
    
    TimePoint snapToNearestPoint(TimePoint position) const;
};

}
