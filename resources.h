// Class to store CPU, disk and tape resources
// Y. Schutz Novembre 2016

#ifndef RESOURCES_H
#define RESOURCES_H

#include <QObject>

class Resources : public QObject
{
    Q_OBJECT
    Q_ENUMS (Cpu_Unit)
    Q_ENUMS (Storage_Unit)
public:

    enum Cpu_Unit {HEPSPEC06, kHEPSPEC06};
    enum Resources_type {kCPU, kDISK, kTAPE};
    Q_ENUM (Resources_type)
    enum Storage_Unit {B, kB, MB, GB, TB, PB};

    explicit Resources(QObject *parent = 0);
    Resources(const QString name);
    Resources(double cpu, Cpu_Unit cpuU, double disk, Storage_Unit diskU, double tape, Storage_Unit tapeU);
    Resources( const Resources& other );

    QString list() const;
    void   clear() { mCPU = 0.0; mDisk = 0.0; mTape = 0.0; }
    double getCPU()  const { return mCPU; }
    double getDisk() const { return mDisk; }
    double getTape() const { return mTape; }
    void   setCPU(double cpu, Cpu_Unit cpuU = kHEPSPEC06);
    void   setDisk(double disk, Storage_Unit diskU = PB);
    void   setTape(double tape, Storage_Unit tapeU = PB);
    void   setTotCPUs(double cpu, Cpu_Unit cpuU = kHEPSPEC06);
    void   setTotDisks(double disk, Storage_Unit diskU = PB);
    void   setTotTapes(double tape, Storage_Unit diskU = PB);

    Resources& operator=( const Resources& other ) {
        mCPU = other.mCPU;
        mDisk = other.mDisk;
        mTape = other.mTape;
        return *this;
    }
private:
    double mCPU;      // CPU resources in  kHEPSPEC06
    double mDisk;     // Disk resources in TBytes
    double mTape;     // Tape resources in TBytes
};

#endif // RESOURCES_H
