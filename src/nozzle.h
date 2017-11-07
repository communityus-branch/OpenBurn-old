#pragma once
#include <QJsonObject>

class OpenBurnNozzle
{
public:
    OpenBurnNozzle(double throat, double exit);
    virtual ~OpenBurnNozzle();
    virtual void SetNozzleThroat(double);
    virtual void SetNozzleExit(double);

    virtual double GetNozzleThroatArea() const;
    virtual double GetNozzleThroat() const;
    virtual double GetNozzleExit() const;

    virtual void ReadJSON(const QJsonObject& object) = 0;
    virtual void WriteJSON(QJsonObject &object) = 0;

protected:
    double m_NozzleThroat; //nozzle throat diameter
    double m_NozzleExit; //nozzle exit diameter
};

class ConicalNozzle : public OpenBurnNozzle
{
public:
    ConicalNozzle(double throat, double exit, double halfAngle = 15.0f);
    virtual ~ConicalNozzle();

    void SetHalfAngle(double);
    void SetNozzleThroatLen(double);
    double GetHalfAngle() const;
    double GetThroatLength() const;

    virtual void ReadJSON(const QJsonObject& object) override;
    virtual void WriteJSON(QJsonObject &object) override;

    //TODO: add calculations to account for departures from ideal performance
    //http://rasaero.com/dloads/Departures%20from%20Ideal%20Performance.pdf
private:
    double m_HalfAngle;
    double m_throatLength; //straight cut throat
};
