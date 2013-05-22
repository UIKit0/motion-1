#include "coreapplication.h"
#include "tools.h"
#include <QDebug>
#include <QFileInfo>
#include <QDir>

CoreApplication::CoreApplication(QObject *parent) :
    QObject(parent)
{
    QObject::connect(&vp, SIGNAL(processProgressChanged(float)), this, SIGNAL(processProgressChanged(float)));
    QObject::connect(this, SIGNAL(registerMatlabFunctionPath(QString)), &ev, SLOT(addFunctionLocationToPath(QString)));
    originalVideo = 0;
    newVideo = 0;
}

void CoreApplication::loadOriginalVideo(QString path)
{
    emit processStatusChanged(CoreApplication::LOAD_VIDEO, true);
    clear();
    cv::VideoCapture vc;
    if (!vc.open(path.toStdString())) {
        qDebug() << "VideoProcessor::loadVideo - Video could not be opened";
        emit processStatusChanged(CoreApplication::LOAD_VIDEO, false);
        return;
    }
    // Load Video
    int frameCount = vc.get(CV_CAP_PROP_FRAME_COUNT);
    int fps = vc.get(CV_CAP_PROP_FPS);
    originalVideo = new Video(frameCount,fps);
    QFileInfo fileInfo = QFileInfo(path);
    originalVideo->setVideoName(fileInfo.fileName());
    int currentFrame = 0;
    Mat buffer;
    while (vc.read(buffer)) {
        emit processProgressChanged((float)currentFrame/frameCount);
        Frame* newFrame = new Frame(buffer.clone(),originalVideo);
        originalVideo->appendFrame(newFrame);
        currentFrame++;
    }
    emit originalVideoLoaded(originalVideo);
    emit processStatusChanged(CoreApplication::LOAD_VIDEO, false);
}

void CoreApplication::saveNewVideo(QString path)
{
    emit processStatusChanged(CoreApplication::SAVE_VIDEO, true);
    VideoWriter record(path.toStdString(), CV_FOURCC('D','I','V','X'),originalVideo->getOrigFps(), newVideo->getSize());
    assert(record.isOpened());
    for (int f = 0; f < newVideo->getFrameCount(); f++) {
        emit processProgressChanged((float)f/newVideo->getFrameCount());
        const Frame* frame = newVideo->getFrameAt(f);
        const Mat& img = frame->getOriginalData();
        record << img;
    }
    emit processStatusChanged(CoreApplication::SAVE_VIDEO, false);
}

void CoreApplication::calculateOriginalMotion()
{
    emit processStatusChanged(CoreApplication::FEATURE_DETECTION, true);
    vp.detectFeatures(originalVideo);
    emit processStatusChanged(CoreApplication::FEATURE_DETECTION, false);
    emit processStatusChanged(CoreApplication::FEATURE_TRACKING, true);
    vp.trackFeatures(originalVideo);
    emit processStatusChanged(CoreApplication::FEATURE_TRACKING, false);
    emit processStatusChanged(CoreApplication::OUTLIER_REJECTION, true);
    vp.rejectOutliers(originalVideo);
    emit processStatusChanged(CoreApplication::OUTLIER_REJECTION, false);
    emit processStatusChanged(CoreApplication::ORIGINAL_MOTION, true);
    vp.calculateMotionModel(originalVideo);
    emit processStatusChanged(CoreApplication::ORIGINAL_MOTION, false);
    saveOriginalGlobalMotionToMatlab();
}

void CoreApplication::calculateNewMotion() {
    emit processStatusChanged(CoreApplication::NEW_MOTION, true);
    vp.calculateUpdateTransform(originalVideo);
    emit processStatusChanged(CoreApplication::NEW_MOTION, false);
    emit processStatusChanged(CoreApplication::NEW_VIDEO, true);
    newVideo = new Video(originalVideo->getFrameCount());
    vp.applyCropTransform(originalVideo, newVideo);
    emit newVideoCreated(newVideo);
    emit processStatusChanged(CoreApplication::NEW_VIDEO, false);
    saveNewGlobalMotionToMatlab();
}

void CoreApplication::calculateSalientUpdateTransform() {
    emit processStatusChanged(CoreApplication::NEW_MOTION, true);
    vp.calculateSalientUpdateTransform(originalVideo);
    emit processStatusChanged(CoreApplication::NEW_MOTION, false);
    emit processStatusChanged(CoreApplication::NEW_VIDEO, true);
    newVideo = new Video(originalVideo->getFrameCount());
    vp.applyCropTransform(originalVideo, newVideo);
    emit newVideoCreated(newVideo);
    emit processStatusChanged(CoreApplication::NEW_VIDEO, false);
    saveNewGlobalMotionToMatlab();
}


void CoreApplication::evaluateNewMotion() {
    qDebug() << "evaluateNewMotion - Not yet implemented";
}

void CoreApplication::setOriginalPointMotion(QMap<int, QPoint> locations) {
    emit processStatusChanged(CoreApplication::PLOTTING_MOVEMENT, true);
    originalPointMotion.clear();
    // Convert QPoint to Point2f
    QMapIterator<int, QPoint> i(locations);
    while (i.hasNext()) {
        i.next();
        QPoint p = i.value();
        Point2f pNew = Tools::QPointToPoint2f(p);
        originalPointMotion.insert(i.key(),pNew);
    }
    // Normalise
    originalPointMotion = Tools::moveToOriginDataSet(originalPointMotion);
    emit processStatusChanged(CoreApplication::PLOTTING_MOVEMENT, false);
}

void CoreApplication::drawGraph(bool usePointOriginal, bool showOriginal, bool showNew, bool x, bool y)
{
    // Build Original Path Object
    QList<Mat> original;
    if (!usePointOriginal) {
        for (int f = 1; f < originalVideo->getFrameCount(); f++) {
            Frame* frame = originalVideo->accessFrameAt(f);
            const Mat& aff = frame->getAffineTransform();
            original.push_back(aff);
        }
    } else {
        qDebug() << "CoreApplication::drawGraph - Not yet compatible with originalPoint";
        return;
    }

    // Build Update Transform Object
    QList<Mat> update;
    if (showNew) {
        for (int f = 1; f < originalVideo->getFrameCount(); f++) {
            Frame* frame = originalVideo->accessFrameAt(f);
            const Mat& aff = frame->getUpdateTransform();
            update.push_back(aff);
        }
    }

    if (showNew) {
        ev.drawNewPath(original, update, showOriginal, x, y);
    } else if (showOriginal) {
        ev.drawOriginalPath(original, x, y);
    }
}

void CoreApplication::saveOriginalGlobalMotionToMatlab() {
    qDebug() << "Saving original motion to Matlab";
    QList<Mat> matrices;
    for (int f = 1; f < originalVideo->getFrameCount(); f++) {
        Frame* frame = originalVideo->accessFrameAt(f);
        matrices.push_back(frame->getAffineTransform());
    }
    QString filePath = QDir::homePath()+QDir::separator()+"motionDump.mat";
    ev.exportMatrices(matrices, filePath, "originalGlobalMotion");
}

void CoreApplication::saveNewGlobalMotionToMatlab() {
    qDebug() << "Saving new motion to Matlab";
    QList<Mat> matrices;
    for (int f = 1; f < originalVideo->getFrameCount(); f++) {
        Frame* frame = originalVideo->accessFrameAt(f);
        matrices.push_back(frame->getUpdateTransform());
    }
    QString filePath = QDir::homePath()+QDir::separator()+"motionDump.mat";
    ev.exportMatrices(matrices, filePath, "newGlobalMotion");
}


void CoreApplication::clear() {
    delete(originalVideo);
    delete(newVideo);
    originalPointMotion.clear();
}

void CoreApplication::setGFTTDetector() {
    vp.setGFTTDetector();
}

void CoreApplication::setSURFDetector() {
    vp.setSURFDetector();
}

void CoreApplication::setSIFTDetector() {
    vp.setSIFTDetector();
}

void CoreApplication::setFASTDetector() {
    vp.setFASTDetector();
}

void CoreApplication::setGFTTHDetector() {
    vp.setGFTTHDetector();
}
