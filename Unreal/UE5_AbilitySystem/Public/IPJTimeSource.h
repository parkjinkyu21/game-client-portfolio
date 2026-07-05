#pragma once
#include "CoreMinimal.h"

// 동기화된 "현재 시간"(초)을 코어에 주입하기 위한 순수 C++ 인터페이스.
// 코어는 이 인터페이스만 조회하며, 로컬/서버/개선클럭/목 중 무엇이 꽂혔는지 모른다.
class IPJTimeSource
{
public:
    virtual ~IPJTimeSource() = default;
    virtual double GetTimeSeconds() const = 0;
};
