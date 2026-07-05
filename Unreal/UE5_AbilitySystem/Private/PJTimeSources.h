#pragma once
#include "IPJTimeSource.h"
#include "UObject/WeakObjectPtr.h"

class UWorld;

class FPJMockTimeSource : public IPJTimeSource
{
public:
    double Current = 0.0;
    void SetTime(double InTime) { Current = InTime; }
    virtual double GetTimeSeconds() const override { return Current; }
};

class FPJLocalTimeSource : public IPJTimeSource
{
public:
    explicit FPJLocalTimeSource(UWorld* InWorld);
    virtual double GetTimeSeconds() const override;
private:
    TWeakObjectPtr<UWorld> World;
};

class FPJServerTimeSource : public IPJTimeSource
{
public:
    explicit FPJServerTimeSource(UWorld* InWorld);
    virtual double GetTimeSeconds() const override;
private:
    TWeakObjectPtr<UWorld> World;
};
