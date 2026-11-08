#include "midi_pitchwheel.h"

#include <QSignalBlocker>

MIDIPitchWheel::MIDIPitchWheel(QObject *parent) : QObject(parent)
{
    this->label_tether = new QLabel("Pitch Tether Strength");
    this->label_pitch = new QLabel("Pitch");

    this->slider_pitch = new QSlider(Qt::Horizontal);
    this->slider_tether = new QSlider(Qt::Horizontal);

    this->slider_tether->setRange(0, 254);
    this->slider_tether->setTickInterval(64);
    this->slider_tether->setTickPosition(QSlider::TicksBelow);
    this->slider_tether->setValue(64);

    // see the midi-protocol, e.g:
    // https://sites.uci.edu/camp2014/2014/04/30/managing-midi-pitchbend-messages/
    this->slider_pitch->setRange(0, MAXIMUM);
    this->slider_pitch->setTickInterval(4096);
    this->slider_pitch->setTickPosition(QSlider::TicksBelow);
    this->slider_pitch->setValue(CENTER);
    this->slider_pitch->setObjectName("slider_pitch");

    this->slider_tether->setTracking(false);
    connect(this->slider_tether, &QSlider::valueChanged, this, &MIDIPitchWheel::tetherChanged);

    this->slider_pitch->setTracking(false);
    connect(this->slider_pitch, &QSlider::sliderMoved, this, &MIDIPitchWheel::sliderMoved);

    this->timer = new QTimer(this);
    this->timer->setInterval(3);
    this->timer->setTimerType(Qt::PreciseTimer);
    connect(this->timer, &QTimer::timeout, this, &MIDIPitchWheel::tick);

    updateLabel(CENTER);
}

void MIDIPitchWheel::movePitchSlider(int position)
{
    const QSignalBlocker blocker(this->slider_pitch);
    this->slider_pitch->setValue(position);

    updateLabel(position);

    emit pitchWheelMoved(position);
}
void MIDIPitchWheel::updateLabel(int position)
{
    /*
    Standard MIDI uses a pitch wheel range of +/-2 semitones = 200 cents. MIDI pitch bend wheel resolution (according to the spec) is +8192/-8191. This means there are 8192/200 = 40.96 pitch bend units to 1 cent.

    http://www.elvenminstrel.com/music/tuning/reference/pitchbends.shtml
    */
    float CENT_FACTOR = 40.96;
    float cent = position / CENT_FACTOR - 200; // -200 to center at 0
    QString indicator = "";
    if (position == CENTER)
        indicator = "";
    else if (position < CENTER && position >= 4096)
        indicator = "<<";
    else if (position > CENTER && position <= CENTER + 4096)
        indicator = ">>";
    else if (position < 4096)
        indicator = "<< <<";
    else if (position > CENTER + 4096)
        indicator = ">> >>";

    this->label_pitch->setText("Pitch: "+indicator+" "+QString::number(cent)+" Cents");
}

void MIDIPitchWheel::sliderMoved(int position)
{
    updateLabel(position);

    emit pitchWheelMoved(position);
}

void MIDIPitchWheel::movePitchWheel(int key)
{
    int value = CENTER;

    if (key == Qt::Key_Left)
    {
        value = 4096;
    }
    else if (key == Qt::Key_Right)
    {
        value = 12288;
    }
    else if (key == Qt::Key_Down)
    {
        value = 0;
    }
    else if (key == Qt::Key_Up)
    {
        value = MAXIMUM;
    }

    movePitchSlider(value);
}

void MIDIPitchWheel::pitchKeyPressed(int key)
{
    this->tether = this->slider_tether->value();
    this->pitch = this->slider_pitch->value();
    this->sign_positive = (this->pitch >= CENTER);

    int direction = 0;
    if (key == KeysRaw::Left)
    {
        direction = -1;
        this->reset_slider = true;
    }
    else if (key == KeysRaw::Right)
    {
        direction = 1;
        this->reset_slider = true;
    }
    else if (key == KeysRaw::End)
    {
        direction = -1;
        this->reset_slider = false;
    }
    else if (key == KeysRaw::Home)
    {
        direction = 1;
        this->reset_slider = false;
    }

    if (!this->reset_slider && this->tether > 0)
    {
        // we want to move the slider 8 times slower if using the home/end keys
        this->tether = qMax(1, this->tether / 8);
    }

    this->direction = direction;
    if (this->direction < 0)
        this->sign_positive = false;
    else if (this->direction > 0)
        this->sign_positive = true;

    this->timer->start();
}
void MIDIPitchWheel::pitchKeyReleased()
{
    // the timer keeps running to let the wheel spring back (if reset_slider)
    this->direction = 0;
}

void MIDIPitchWheel::tick()
{
    if (this->direction == 0)
    {
        if (this->pitch == CENTER || !this->reset_slider)
        {
            this->timer->stop();
            return;
        }

        // spring back towards the center
        if (this->pitch < CENTER)
            this->pitch += this->tether;
        else
            this->pitch -= this->tether;

        // if we shoot over the center we stop exactly there
        if ((this->sign_positive && this->pitch < CENTER) || (!this->sign_positive && this->pitch > CENTER))
        {
            this->pitch = CENTER;
            this->timer->stop();
        }

        movePitchSlider(this->pitch);
    }
    else
    {
        // moved as long as the key is pressed, within the valid range
        this->pitch = qBound(0, this->pitch + this->direction * this->tether, MAXIMUM);

        movePitchSlider(this->pitch);
    }
}

QMap<QString,int> MIDIPitchWheel::getData()
{
    QMap<QString,int> result;

    result["pitchwheel_tether"] = this->slider_tether->value();
    result["pitchwheel_pitch"] = this->slider_pitch->value();

    return result;
}
void MIDIPitchWheel::setData(QMap<QString,QVariant> data)
{
    this->timer->stop();
    this->direction = 0;

    const QSignalBlocker block_tether(this->slider_tether);
    const QSignalBlocker block_pitch(this->slider_pitch);

    if (data.contains("pitchwheel_tether"))
        this->slider_tether->setValue(data["pitchwheel_tether"].toInt());
    if (data.contains("pitchwheel_pitch"))
        this->slider_pitch->setValue(data["pitchwheel_pitch"].toInt());

    this->pitch = this->slider_pitch->value();
    updateLabel(this->pitch);
}
