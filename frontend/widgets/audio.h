#ifndef AUDIO_H
#define AUDIO_H

#include <QWidget>
#include <QPainter>

class VU : public QWidget
{
    Q_OBJECT
public:
    VU(QWidget *parent = nullptr) : QWidget(parent), level(0) {}

    void setLevel(float value) {
        level = value;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Fundo preto
        painter.fillRect(rect(), Qt::black);

        // Define a cor do VU Meter (gradiente)
        QLinearGradient gradient(0, 0, 0, height());
        gradient.setColorAt(0.0, Qt::green);
        gradient.setColorAt(1.0, Qt::red);
        painter.setBrush(gradient);

        // Calcula a altura da barra com base no nível
        int barHeight = static_cast<int>(height() * level);
        painter.drawRect(0, height() - barHeight, width(), barHeight);
    }

private:
    float level;
signals:
};

class Audio : public QWidget
{
    Q_OBJECT
public:
    explicit Audio(QWidget *parent = nullptr);

signals:
};

#endif // AUDIO_H
