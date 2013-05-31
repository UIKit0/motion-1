#ifndef L1SALIENTMODEL_H
#define L1SALIENTMODEL_H

#include "video.h"
#include "l1model.h"

class L1SalientModel : public L1Model
{
public:
    int salientSlackVarPerFrame;


    L1SalientModel(int frameCount);

    // This is the entry function
    void prepare(Video* video, bool centered);

    // Feature Transform Variables, Slack Variables, Salient Slack Variables
    void setObjectives();

    // Helper Function
    int toSalientSlackIndex(int t, int corner, char component);

    // Overrides L1Model function
    // Ensures Cropbox is always within transformed frame.
    void setInclusionConstraints(Rect cropbox, int videoWidth, int videoHeight);

    void setSalientConstraints(Video* video, bool centered);

};

#endif // L1SALIENTMODEL_H
