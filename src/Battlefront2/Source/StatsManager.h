#pragma once

struct LeaderBoardStatsT
{
    __int64 rating;
    int shotsFired;
    int shotsHit;
    int criticalshots;
    int livingStreak;
    int narrowEscapes;
    int kills;
    int deaths;
    int captures;
    int vehiclesDestroyed;
    int lastPlayed;
    int totalPlayingTime;
    int wins;
    int losses;
    int tankBuster;
    int killingSpree;
    int camper;
    int survivalist;
    int traitor;
    int publicEnemy;
    int meat;
    int deadEye;
    int pointman;
    int points;
    int hero_points;
    int game_start;
    int game_finish;
    int game_rank;
    unsigned int favoriteClassHash;
    int favoriteClassTime;
    unsigned int favoriteVehicleHash;
    int favoriteVehicleTime;
    unsigned int favoriteWeaponHash;
    int favoriteWeaponTime;
    char nemesis[32];
    char bait[32];
    int nemesisDeaths;
    int baitKills;
};




struct LeaderBoardPlayerStatsT
{
    char username[32];
    int level;
    unsigned int titleHash;
    LeaderBoardStatsT factionStats[4];
    LeaderBoardStatsT overallStats;
};
