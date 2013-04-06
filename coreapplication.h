#ifndef COREAPPLICATION_H
#define COREAPPLICATION_H

#include <QObject>
#include "videoprocessor.h"
#include "evaluator.h"
#include <QMap>

class CoreApplication : public QObject
{
    Q_OBJECT
public:
    explicit CoreApplication(QObject *parent = 0);

    const static int FEATURE_DETECTION = 1;
    const static int FEATURE_TRACKING = 2;
    const static int OUTLIER_REJECTION = 3;
    const static int ORIGINAL_MOTION = 4;
    const static int LOAD_VIDEO = 5;
    const static int NEW_MOTION = 6;
    const static int NEW_VIDEO = 10;
    const static int SAVE_VIDEO = 11;
    const static int ANALYSE_CROP_VIDEO = 12;
    const static int PLOTTING_MOVEMENT = 13;
    
signals:
    void processStatusChanged(int,bool);
    void processProgressChanged(float);
    void originalVideoLoaded(Video* video);
    void newVideoCreated(Video* video);
    
public slots:
    void loadOriginalVideo(QString path);
    void saveNewVideo(QString path);
    void calculateOriginalMotion();
    void calculateNewMotion();
    void evaluateNewMotion();
    void drawGraph();

    void setOriginalPointMotion(QMap<int, QPoint> locations);
    void setOriginalGlobalMotion();
    void setNewGlobalMotion();

    void saveOriginalGlobalMotionToMatlab();
    void saveNewGlobalMotionToMatlab();

    void drawGraph(bool originalPointMotion, bool originalGlobalMotion, bool newGlobalMotion, bool x, bool y);


private:
    // For Loading Video and Processing it
    VideoProcessor vp;
    Video* originalVideo;
    Video* newVideo;

    // For Evaluating Video and drawing Graphs
    Evaluator ev;
    QMap<int, Point2f> originalPointMotion;
    QMap<int, Point2f> originalGlobalMotion;
    QMap<int, Point2f> newGlobalMotion;

    void clear();



};

#endif // COREAPPLICATION_H
