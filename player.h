#ifndef PLAYER_H
#define PLAYER_H
#include <video.h>
#include <frame.h>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QImage>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
using namespace cv;

class Player : public QThread
{
    Q_OBJECT
private:
    bool stopped;
    QMutex mutex;
    QWaitCondition condition;
    Mat frame;
    int frameRate;
    Mat RGBframe;
    QImage image;
    Video video;
    int frameNumber;

    bool featuresEnabled;
    bool trackedEnabled;
    bool outliersEnabled;

    void showImage(int frameNumber);

signals:
    void processedImage(const QImage &image, int frameNumber);
    void playerStopped();
protected:
    void run();
    void msleep(int ms);
public slots:
    void setVideo(const Video& video);
public:
    Player(QObject *parent = 0);
    ~Player();
    void play();
    void step();
    void stop();
    void rewind();
    void refresh();
    bool isStopped() const;
    void setFrameRate(int frameRate);
    const int getFrameRate() const;

    void setFeaturesEnabled(bool enabled);
    void setTrackingEnabled(bool enabled);
    void setOutliersEnabled(bool enabled);
};

#endif // PLAYER_H
