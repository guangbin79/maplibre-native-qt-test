#pragma once
#include <QWidget>
#include <vector>

class ScaleBarWidget : public QWidget {
    Q_OBJECT
public:
    explicit ScaleBarWidget(QWidget *parent = nullptr);

public slots:
    void updateScale(double latitude, double zoomLevel);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double m_latitude = 36.75;
    double m_zoomLevel = 8.0;
    std::vector<int> m_niceDistances;
};
