/**
 * Copyright (c) 2023 Dmitry Ivanov
 *
 * This file is a part of QEverCloud project and is distributed under the terms
 * of MIT license: https://opensource.org/licenses/MIT
 */

#include "NonceGenerator.h"

#include <QByteArray>
#include <QRandomGenerator>
#include <QUuid>

#include <cstring>

namespace qevercloud {

namespace {

[[nodiscard]] quint64 random64()
{
    quint64 res = 0;
    for(int i = 0; i < 8; i++) {
        res += (QRandomGenerator::global()->generate64() % 256) << i*8;
    }

    QByteArray randomData = QUuid::createUuid().toRfc4122();
    quint64 random;
    std::memcpy(&random, &randomData.constData()[0], sizeof(random));

    res ^= random;

    std::memcpy(
        &random,
        &randomData.constData()[sizeof(random)],
        sizeof(random));

    res ^= random;
    return res;
}

NonceGenerator nonceGenerator_ = random64;

} // namespace

NonceGenerator nonceGenerator()
{
    return nonceGenerator_;
}

void setNonceGenerator(quint64 (*nonceGenerator)())
{
    nonceGenerator_ = nonceGenerator;
}

} // namespace qevercloud
