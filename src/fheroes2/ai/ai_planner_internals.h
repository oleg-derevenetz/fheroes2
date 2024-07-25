/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2024                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#pragma once

class Castle;
class Heroes;

namespace AI
{
    const double ARMY_ADVANTAGE_DESPERATE = 0.8;
    const double ARMY_ADVANTAGE_SMALL = 1.3;
    const double ARMY_ADVANTAGE_MEDIUM = 1.5;
    const double ARMY_ADVANTAGE_LARGE = 1.8;

    class AIWorldPathfinderStateRestorer
    {
    public:
        explicit AIWorldPathfinderStateRestorer( AIWorldPathfinder & pathfinder )
            : _pathfinder( pathfinder )
            , _originalMinimalArmyStrengthAdvantage( _pathfinder.getMinimalArmyStrengthAdvantage() )
            , _originalSpellPointsReserveRatio( _pathfinder.getSpellPointsReserveRatio() )
        {}

        AIWorldPathfinderStateRestorer( const AIWorldPathfinderStateRestorer & ) = delete;

        ~AIWorldPathfinderStateRestorer()
        {
            _pathfinder.setMinimalArmyStrengthAdvantage( _originalMinimalArmyStrengthAdvantage );
            _pathfinder.setSpellPointsReserveRatio( _originalSpellPointsReserveRatio );
        }

        AIWorldPathfinderStateRestorer & operator=( const AIWorldPathfinderStateRestorer & ) = delete;

    private:
        AIWorldPathfinder & _pathfinder;

        const double _originalMinimalArmyStrengthAdvantage;
        const double _originalSpellPointsReserveRatio;
    };

    struct AICastle
    {
        Castle * castle = nullptr;
        bool underThreat = false;
        int safetyFactor = 0;
        int buildingValue = 0;

        AICastle( Castle * inCastle, bool inThreat, int inSafety, int inValue )
            : castle( inCastle )
            , underThreat( inThreat )
            , safetyFactor( inSafety )
            , buildingValue( inValue )
        {
            assert( castle != nullptr );
        }
    };

    struct EnemyArmy
    {
        EnemyArmy() = default;

        EnemyArmy( const int32_t index_, const int color_, const Heroes * hero_, const double strength_, const uint32_t movePoints_ )
            : index( index_ )
            , color( color_ )
            , hero( hero_ )
            , strength( strength_ )
            , movePoints( movePoints_ )
        {}

        int32_t index{ -1 };
        int color{ Color::NONE };
        const Heroes * hero{ nullptr };
        double strength{ 0 };
        uint32_t movePoints{ 0 };
    };

    struct HeroToMove
    {
        Heroes * hero = nullptr;
        int patrolCenter = -1;
        uint32_t patrolDistance = 0;
    };

    // TODO: this structure is not being updated during AI heroes' actions.
    struct RegionStats
    {
        bool evaluated = false;
        double highestThreat = -1;
        int friendlyHeroes = 0;
        int friendlyCastles = 0;
        int enemyCastles = 0;
        int safetyFactor = 0;
        int spellLevel = 2;
    };
}
