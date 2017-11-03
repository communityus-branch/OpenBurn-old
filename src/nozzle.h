#pragma once


class OpenBurnNozzle
{
public:
    OpenBurnNozzle(double throat, double exit)
        : m_NozzleThroat(throat), m_NozzleExit(exit)
    {}
    ~OpenBurnNozzle() {}

    virtual double GetNozzleThroatArea() = 0;
    virtual double GetNozzleThroat() { return m_NozzleThroat; }
    virtual double GetNozzleExit() { return m_NozzleExit; }
protected:
    double m_NozzleThroat; //nozzle throat diameter
    double m_NozzleExit; //nozzle exit diameter
};

class ConicalNozzle : public OpenBurnNozzle
{
public:
    ConicalNozzle(double throat, double exit, double halfAngle = 15.0f)
        : OpenBurnNozzle(throat, exit), m_HalfAngle(halfAngle)
    {}
    ~ConicalNozzle();
    virtual double GetNozzleThroatArea() override;

    void SetNozzleThroat(double);
    void SetNozzleExit(double);
    void SetHalfAngle(double);
    void SetNozzleThroatLen(double);
    double GetHalfAngle() { return m_HalfAngle; }
    double GetThroatLength() { return m_throatLength; }

    //TODO: add calculations to account for departures from ideal performance
    //http://rasaero.com/dloads/Departures%20from%20Ideal%20Performance.pdf
private:
    double m_HalfAngle;
    double m_throatLength; //straight cut throat
};
