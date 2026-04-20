#pragma once
#include <QWidget>

class QSlider;
class QLabel;

class ControlPanelWidget : public QWidget {
    Q_OBJECT
public:
    explicit ControlPanelWidget(QWidget *parent = nullptr);

signals:
    void zoomChanged(double zoom);
    void bearingChanged(double bearing);
    void tiltChanged(double tilt);

public slots:
    void setZoomValue(double zoom);
    void setBearingValue(double bearing);
    void setTiltValue(double tilt);

private:
    QSlider *m_zoomSlider;
    QSlider *m_bearingSlider;
    QSlider *m_tiltSlider;
    QLabel *m_zoomLabel;
    QLabel *m_bearingLabel;
    QLabel *m_tiltLabel;
};
