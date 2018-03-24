// xtensor-interpolate: https://github.com/rjsberry/xtensor-interpolate
//
// Copyright (C) 2018, Richard Berry <rjsberry@protonmail.com>
//
// Distributed under the terms of BSD 2-Clause "simplified" license. (See
// accompanying file LICENSE, or copy at
// https://github.com/rjsberry/xtensor-interpolate/blob/master/LICENSE)
//

#include "gtest/gtest.h"

#define XTENSOR_ENABLE_ASSERT
#include "xtensor/xexception.hpp"
#include "xtensor/xmath.hpp"
#include "xtensor/xrandom.hpp"
#include "xtensor/xstrided_view.hpp"
#include "xtensor/xtensor.hpp"

#include "xtensor-interpolate/xinterpolate.hpp"

namespace xt
{

namespace { const std::size_t ARR_LEN = 100; }

// Smoke tests

TEST(smoke_test, splrep_splev)
{
    xtensor<double, 1> x = linspace<double>(0, M_PI, 10);
    xtensor<double, 1> y = sin(x);

    auto tck = interpolate::splrep(x, y);

    xtensor<double, 1> xs = arange<double>(0, M_PI, 100);
    xtensor<double, 1> ys = interpolate::splev(xs, tck);

    EXPECT_TRUE(allclose(ys, sin(xs), 0.00001));
}

TEST(smoke_test, splrep_splint)
{
    struct TestCase
    {
        xtensor<double, 1>                                    x;
        std::function<xtensor<double, 1>(xtensor<double, 1>)> f;
        double                                                definite_integral;
    };

    std::vector<TestCase> tests =
    {
        { linspace<double>(0, 2*M_PI, ARR_LEN), [](auto x){ return sin(x); }, 0. },
        { linspace<double>(0., 1., ARR_LEN), [](auto x){ return 3 * x * x; }, 1. },
        { linspace<double>(-1., 1., ARR_LEN), [](auto x){ return 1/(1 + 25*(x*x)); }, 0.54936 },
    };

    for (const auto& test : tests)
    {
        xtensor<double, 1> x = test.x;
        xtensor<double, 1> y = test.f(test.x);
        auto tck = interpolate::splrep(x, y);

        EXPECT_TRUE(allclose(interpolate::splint(test.x[0], test.x[test.x.shape()[0] - 1], tck),
                             test.definite_integral,
                             0.00001));
    }
}

TEST(smoke_test, splrep_spalde)
{
    // Linear
    xtensor<double, 1> x = linspace<double>(0, ARR_LEN/10, ARR_LEN);
    xtensor<double, 1> y = x;

    auto tck = interpolate::splrep(x, y);

    xtensor<double, 1> dx = arange<double>({ ARR_LEN/10 });
    auto dy = interpolate::spalde(dx, tck);

    for (std::size_t i = 0; i < ARR_LEN/10; ++i)
    {
        xtensor<double, 1> expected = { dx[i], 1, 0, 0 };

        EXPECT_TRUE(allclose(expected,
                             flatten(view(dy, range(i, i+1), all())),
                             0.00001));
    }
}

TEST(smoke_test, splrep_splder_splev)
{
    // Linear
    xtensor<double, 1> x = arange<double>(ARR_LEN);
    xtensor<double, 1> y = x;

    auto tck = interpolate::splrep(x, y);
    auto dtck = interpolate::splder(x, tck);
    auto ddtck = interpolate::splder(x, dtck);

    auto dy = interpolate::splev(x, dtck);
    auto ddy = interpolate::splev(x, ddtck);

    xtensor<double, 1> d_expected = ones<double>({ ARR_LEN });
    xtensor<double, 1> dd_expected = zeros<double>({ ARR_LEN });

    EXPECT_TRUE(allclose(d_expected, dy, 0.00001));
    EXPECT_TRUE(allclose(dd_expected, ddy, 0.00001));
}

// Unit tests

TEST(splder, invalid_nu_throws)
{
    xtensor<double, 1> x = random::randn<double>({ ARR_LEN });

    xtensor<double, 1> t = random::randn<double>({ ARR_LEN });
    xtensor<double, 1> c = random::randn<double>({ ARR_LEN });
    auto k = random::randint({ 1 }, 1, 6)[0];
    
    auto nu = k + 1;  // Invalid derivative.

    try
    {
        auto dtck = interpolate::splder(x, std::make_tuple(t, c, k), nu);
        FAIL() << "Expected `std::invalid_argument`";
    }
    catch (const std::invalid_argument& e)
    {
        EXPECT_EQ(std::string(e.what()), "order of derivative must be <= k");
    }
    catch (...)
    {
        FAIL() << "Expected `std::invalid_argument`";
    }

    
    for (const auto& nu : { k, k - 1 })  // Valid derivatives.
    {
        try
        {
            auto dtck = interpolate::splder(x, std::make_tuple(t, c, k), nu);
        }
        catch (const std::invalid_argument&)
        {
            FAIL() << "Did not expected `std::invalid_argument`";
        }
        catch (const std::runtime_error&)
        {
            ;  // Don't care if randomly generated input error is invalid.
        }
        catch (...)
        {
            FAIL() << "An unknown error occurred";
        }
    }
}

}  // namespace xt
