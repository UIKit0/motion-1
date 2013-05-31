#ifndef TOOLS_H
#define TOOLS_H
#include <opencv2/core/core.hpp>
#include "video.h"
#include "frame.h"
#include "evaluator.h"

using namespace cv;

class Tools
{
public:
    Tools();
    static float eucDistance(Point2f a, Point2f b);
    static Point2f applyAffineTransformation(Mat affine, Point2f src);
    static RotatedRect transformRectangle(const Mat& affine, const Rect& origRect);
    static Mat getCroppedImage(const Mat& image, const RotatedRect& rect);
    static Point2f QPointToPoint2f(QPoint p);

    // Move coordinates to start from 0,0
    static QMap<int, Point2f> moveToOriginDataSet(const QMap<int, Point2f>& dataSet);

    // Builds coordinates from original manual motion

    // Builds coordinates from original video global motion

    // Builds coordinates with motion update and then camera update
};

#endif // TOOLS_H
