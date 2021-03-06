#include "coreapplication.h"
#include "tools.h"
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include "frame.h"

CoreApplication::CoreApplication(QObject *parent) :
    QObject(parent)
{
    QObject::connect(&vp, SIGNAL(processProgressChanged(float)), this, SIGNAL(processProgressChanged(float)));
    QObject::connect(this, SIGNAL(registerMatlabFunctionPath(QString)), &ev, SLOT(addFunctionLocationToPath(QString)));
    originalVideo = 0;
    newVideo = 0;
}

Video* CoreApplication::loadOriginalVideo(QString path)
{
    emit processStatusChanged(CoreApplication::LOAD_VIDEO, true);
    clear();
    cv::VideoCapture vc;
    if (!vc.open(path.toStdString())) {
        qDebug() << "CoreApplication::loadVideo - Video could not be opened";
        emit processStatusChanged(CoreApplication::LOAD_VIDEO, false);
        return 0;
    }
    // Load Video
    int frameCount = vc.get(CV_CAP_PROP_FRAME_COUNT);
    int fps = vc.get(CV_CAP_PROP_FPS);
    originalVideo = new Video(frameCount,fps);
    QFileInfo fileInfo = QFileInfo(path);
    originalVideo->setVideoName(fileInfo.fileName());
    int currentFrame = 0;
    Mat buffer;
    while (true) {
        emit processProgressChanged((float)currentFrame/frameCount);
        vc >> buffer;
        if (buffer.empty()) {
            break;
        }
        Frame* newFrame = new Frame(buffer.clone(),originalVideo);
        originalVideo->appendFrame(newFrame);
        currentFrame++;
    }
    if(currentFrame != frameCount) {
        qWarning() << "Warning: May not have read in all frames";
    }
    videoFourCCCodec = static_cast<int>(vc.get(CV_CAP_PROP_FOURCC));
    emit originalVideoLoaded(originalVideo);
    emit processStatusChanged(CoreApplication::LOAD_VIDEO, false);
    return originalVideo;
}

void CoreApplication::saveNewVideo(QString path)
{
    emit processStatusChanged(CoreApplication::SAVE_VIDEO, true);
    VideoWriter record(path.toStdString(), videoFourCCCodec,25, newVideo->getSize());
    assert(record.isOpened());
    for (int f = 0; f < newVideo->getFrameCount(); f++) {
        emit processProgressChanged((float)f/newVideo->getFrameCount());
        const Frame* frame = newVideo->getFrameAt(f);
        Mat img = frame->getOriginalData().clone();
        record << img;
    }
    emit processStatusChanged(CoreApplication::SAVE_VIDEO, false);
}

void CoreApplication::saveOldVideo(QString path) {
    emit processStatusChanged(CoreApplication::SAVE_VIDEO, true);
    VideoWriter record(path.toStdString(), videoFourCCCodec,25,originalVideo->getSize());
    assert(record.isOpened());
    for (int f = 0; f < originalVideo->getFrameCount(); f++) {
        emit processProgressChanged((float)f/originalVideo->getFrameCount());
        const Frame* frame = originalVideo->getFrameAt(f);
        Mat image = frame->getOriginalData().clone();
        const Rect_<int>& cropBox = originalVideo->getCropBox();
        if (f == 0) {
            cv::rectangle(image, cropBox, Scalar(0,255,0), 3);
        } else {
            const Mat& update = frame->getUpdateTransform();
            RotatedRect newCrop = Tools::transformRectangle(update, cropBox);
            Point2f verts[4];
            newCrop.points(verts);
            for (int i = 0; i < 4; i++) {
                line(image, verts[i], verts[(i+1)%4], Scalar(0,255,0),3);
            }
        }
        record << image;
    }
    emit processStatusChanged(CoreApplication::SAVE_VIDEO, false);
}

void CoreApplication::saveCroppedOldVideo(QString path)
{
    emit processStatusChanged(CoreApplication::SAVE_VIDEO, true);
    //qDebug() << "Saving cropped video " << originalVideo->getCropBox().size().width << "," << originalVideo->getCropBox().size().height;
    Size croppedSize(originalVideo->getCropBox().size().width, originalVideo->getCropBox().size().height);

    VideoWriter record(path.toStdString(), videoFourCCCodec,25,croppedSize);
    assert(record.isOpened());
    for (int f = 0; f < originalVideo->getFrameCount(); f++) {
        emit processProgressChanged((float)f/originalVideo->getFrameCount());
        const Frame* frame = originalVideo->getFrameAt(f);
        Mat originalData = frame->getOriginalData();
        Mat croppedImage;
        Rect cropBox = originalVideo->getCropBox();
        Mat(originalData, cropBox).copyTo(croppedImage);
        record << croppedImage;
    }
    emit processStatusChanged(CoreApplication::SAVE_VIDEO, false);
}


void CoreApplication::calculateOriginalMotion(int radius)
{
    originalVideo->reset();
    emit processStatusChanged(CoreApplication::FEATURE_DETECTION, true);
    vp.detectFeatures(originalVideo, radius);
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
}

void CoreApplication::calculateNewMotion(bool salient, bool centered)
{
    emit processStatusChanged(CoreApplication::NEW_MOTION, true);
    if (!salient) {
        vp.calculateUpdateTransform(originalVideo);
    } else {
        vp.calculateSalientUpdateTransform(originalVideo,centered);
    }
    emit processStatusChanged(CoreApplication::NEW_MOTION, false);
    emit processStatusChanged(CoreApplication::NEW_VIDEO, true);
    newVideo = new Video(originalVideo->getFrameCount());
    vp.applyCropTransform(originalVideo, newVideo);
    emit newVideoCreated(newVideo);
    emit processStatusChanged(CoreApplication::NEW_VIDEO, false);
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

void CoreApplication::saveOriginalGlobalMotionMat(QString path) {
    qDebug() << "Saving original motion to Matlab";
    QList<Mat> matrices;
    for (int f = 1; f < originalVideo->getFrameCount(); f++) {
        Frame* frame = originalVideo->accessFrameAt(f);
        matrices.push_back(frame->getAffineTransform());
    }
    ev.exportMatrices(matrices, path, "originalGlobalMotion");
}

void CoreApplication::saveNewGlobalMotionMat(QString path) {
    qDebug() << "Saving new motion to Matlab";
    QList<Mat> matrices;
    for (int f = 1; f < originalVideo->getFrameCount(); f++) {
        Frame* frame = originalVideo->accessFrameAt(f);
        matrices.push_back(frame->getUpdateTransform());
    }
    ev.exportMatrices(matrices, path, "newGlobalMotion");
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

void CoreApplication::loadFeatures(QString path) {
    QMap<int, Point2f*> locations;
    QFile file(path);
    if (file.exists()) {
        file.open(QIODevice::ReadOnly | QIODevice::Text);
        QTextStream in(&file);
        while(!in.atEnd()) {
            QString line = in.readLine();
            QStringList lineParts = line.split(",");
            int frame = lineParts.at(0).toInt();
            Point2f* p = new Point2f(lineParts.at(1).toInt(), lineParts.at(2).toInt());
            locations.insert(frame,p);
        }
    }
    QMapIterator<int, Point2f*> i(locations);
    while (i.hasNext()) {
        i.next();
        Frame* f = originalVideo->accessFrameAt(i.key());
        f->setFeature(i.value());
    }
}

void CoreApplication::saveOriginalFrame(QString path, int frame, bool cropped){
    const Frame* f = originalVideo->getFrameAt(frame);
    const Mat& originalData = f->getOriginalData();
    if (cropped) {
        Mat cropped = originalData(originalVideo->getCropBox());
        cv::imwrite(path.toStdString(), cropped);
    } else {
        cv::imwrite(path.toStdString(), originalData);
    }
}

void CoreApplication::saveNewFrame(QString path, int frame) {
    if (frame == 0) {
        saveOriginalFrame(path, frame, true);
        return;
    }
    const Frame* f = originalVideo->getFrameAt(frame);
    const Mat& originalData = f->getOriginalData();
    const Rect& cropBox = originalVideo->getCropBox();
    RotatedRect newCropWindow = Tools::transformRectangle(f->getUpdateTransform(), cropBox);
    Mat newImage = Tools::getCroppedImage(originalData, newCropWindow);
    resize(newImage,newImage,cropBox.size());
    cv::imwrite(path.toStdString(), newImage);
}
