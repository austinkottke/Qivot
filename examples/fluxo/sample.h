#ifndef SAMPLE_H
#define SAMPLE_H
#include <qivot.h>

/// One particle's position at one frame — the row Fluxo writes thousands of,
/// 60 times a second. Everything you see on screen in replay is read back out
/// of this table.
class Sample : public QiModel {
    QI_MODEL
public:
    QiField<int>    frame;   ///< which recorded frame this sample belongs to
    QiField<double> x;       ///< position, in device pixels
    QiField<double> y;
    QiField<int>    hue;     ///< 0..359, the particle's glow color
};

// Field order here is the order QiListWriter streams them per row.
QI_DECLARE_MODEL(Sample, "sample",
    QI_FIELD(frame), QI_FIELD(x), QI_FIELD(y), QI_FIELD(hue));

#endif // SAMPLE_H
