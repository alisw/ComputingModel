// Class to store CPU, disk and tape resources
// Y. Schutz Novembre 2016

#include <QDebug>
#include <QtMath>
#include "resources.h"

//===========================================================================
Resources::Resources(QObject *parent) : QObject(parent),
    mCPU(0.0), mDisk(0.0), mTape(0.0)
{
    // default ctor
    setObjectName("No pledges");
}

//===========================================================================
Resources::Resources(const QString name)
{
    // ctor passing the name
    setObjectName(name);
}

//===========================================================================
Resources::Resources(double cpu, Resources::Cpu_Unit cpuU, double disk, Resources::Storage_Unit diskU, double tape, Resources::Storage_Unit tapeU)
{
    //  ctor with data assignements

    setObjectName("No Name");

    setCPU(cpu, cpuU);

    setDisk(disk, diskU);

    setTape(tape, tapeU);
}

//===========================================================================
Resources::Resources(const Resources &other) : QObject(),
    mCPU(other.mCPU), mDisk(other.mDisk), mTape(other.mTape)
{
    //copy ctor
}

//===========================================================================
QString Resources::list() const
{
    // list the resources
    QString text = QString("Resources: %1").arg(objectName());
    if (objectName() != "No pledges")
        text.append(QString("☛ CPU: %1 kHEPSPEC06 - Disk: %2 PB - Tape %3 PB\n").arg(mCPU, 5, 'f', 2).arg(mDisk, 5, 'f', 2).arg(mTape, 5, 'f', 2));
    else
        text.append("\n");
    return text;
}

//===========================================================================
void Resources::setCPU(double cpu, Resources::Cpu_Unit cpuU)
{
    // CPU is in kHEPSPEC06

    switch (cpuU) {
    case HEPSPEC06:
        mCPU = cpu / 1000.;
        break;
    case kHEPSPEC06:
        mCPU = cpu;
        break;
    default:
        break;
    }
}

//===========================================================================
void Resources::setDisk(double disk, Resources::Storage_Unit diskU)
{
    // disk is in PB

    switch (diskU) {
    case B:
        mDisk = disk / qPow(10, 15);
        break;
    case kB:
        mDisk = disk / qPow(10, 12);
        break;
    case MB:
        mDisk = disk / qPow(10, 9);
        break;
    case GB:
        mDisk = disk / qPow(10, 6);
        break;
    case TB:
        mDisk = disk / qPow(10, 3);
        break;
    case PB:
        mDisk = disk;
        break;
    default:
        break;
    }
}

//===========================================================================
void Resources::setTape(double tape, Resources::Storage_Unit tapeU)
{
    // tape is in PB

    switch (tapeU) {
    case B:
        mTape = tape / qPow(10, 15);
        break;
    case kB:
        mTape = tape / qPow(10, 12);
        break;
    case MB:
        mTape = tape / qPow(10, 9);
        break;
    case GB:
        mTape = tape / qPow(10, 6);
        break;
    case TB:
        mTape = tape / qPow(10, 3);
        break;
    case PB:
        mTape = tape;
        break;
    default:
        break;
    }
}
