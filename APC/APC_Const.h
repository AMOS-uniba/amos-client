//------------------------------------------------------------------------------
//
// APC_Const.h
//
// Purpose:
//
//    Definition of constants used in APC library and applications
//
// Notes:
//
//   This software is protected by national and international copyright.
//   Any unauthorized use, reproduction or modificaton is unlawful and
//   will be prosecuted. Commercial and non-private application of the
//   software in any form is strictly prohibited unless otherwise granted
//   by the authors.
//
// (c) 1999 Oliver Montenbruck, Thomas Pfleger
//
//------------------------------------------------------------------------------

#ifndef INC_APC_CONST_H
#define INC_APC_CONST_H

constexpr double pi        = 3.14159265358979324;
constexpr double pi2       = 2.0*pi;
constexpr double Rad       = pi / 180.0;
constexpr double Deg       = 180.0 / pi;
constexpr double Arcs      = 3600.0 * 180.0/pi;

// Radii of Earth, Sun and Moon
constexpr double R_Earth   =   6378.137;     // [km]
constexpr double R_Sun     = 696000.0;       // [km]
constexpr double R_Moon    =   1738.0;       // [km]

constexpr double MJD_J2000 = 51544.5;        // MJD of Epoch J2000.0
constexpr double T_J2000   =  0.0;           // Epoch J2000.0
constexpr double T_B1950   = -0.500002108;   // Epoch B1950

constexpr double kGauss    = 0.01720209895;  // gravitational constant
constexpr double GM_Sun    = kGauss*kGauss;  // [AU^3/d^2]

constexpr double AU        = 149597870.691;    // Astronomical unit [km]

constexpr double c_light   = 299792.458 / AU * 86400;  // speed of light [AU/d]


#endif  // include blocker
