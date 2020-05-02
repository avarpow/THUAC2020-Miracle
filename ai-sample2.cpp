#include "ai_client.h"
#include "gameunit.h"
#include "card.hpp"
#include "calculator.h"

#include <fstream>
#include <stdlib.h>
#include <random>
#include <time.h>

using gameunit::Pos;
using gameunit::Unit;

using calculator::all_pos_in_map;
using calculator::cube_distance;
using calculator::cube_reachable;
using calculator::reachable;
using calculator::units_in_range;

using card::CARD_DICT;

using std::default_random_engine;
using std::get;
using std::make_pair;
using std::make_tuple;
using std::map;
using std::pair;
using std::string;
using std::uniform_int_distribution;
using std::vector;

class AI : public AiClient
{
private:
    Pos miracle_pos;

    Pos enemy_pos;

    Pos target_barrack;

    Pos enemy_defense_pos1;

    Pos enemy_defense_pos2;

    Pos my_attack_pos1;

    Pos my_attack_pos2;

    Pos my_attack_pos3;

    int id_barracks_sol;

    Pos posShift(Pos pos, string direct, int shift_num);

    int enemy_miracle_hp;
    enum attack_modes
    {
        ATTACK,
        DEFENSE
    } attack_mode;

    std::vector<gameunit::Unit> waitting_allay_list;

public:
    //选择初始卡组
    void chooseCards(); //(根据初始阵营)选择初始卡组

    void play(); //玩家需要编写的ai操作函数

    void battle(); //处理生物的战斗

    void march(); //处理生物的移动
};

void AI::chooseCards()
{
    // (根据初始阵营)选择初始卡组

    /*
     * artifacts和creatures可以修改
     * 【进阶】在选择卡牌时，就已经知道了自己的所在阵营和先后手，因此可以在此处根据先后手的不同设置不同的卡组和神器
     */
    my_artifacts = {"SalamanderShield"};
    my_creatures = {"Archer", "BlackBat", "VolcanoDragon"};
    attack_mode = ATTACK;
    enemy_miracle_hp = 30;
    init();
}

void AI::play()
{
    //玩家需要编写的ai操作函数

    /*
    本AI采用这样的策略：
    在首回合进行初期设置、在神迹优势路侧前方的出兵点召唤一个1星弓箭手
    接下来的每回合，首先尽可能使用神器，接着执行生物的战斗，然后对于没有进行战斗的生物，执行移动，最后进行召唤
    在费用较低时尽可能召唤星级为1的兵，优先度剑士>弓箭手>火山龙
    【进阶】可以对局面进行评估，优化神器的使用时机、调整每个生物行动的顺序、调整召唤的位置和生物种类、星级等
    */
    fprintf(stderr, "round:%d\n", round);
    if (round / 2 == 0)
    {
        //先确定自己的基地、对方的基地
        miracle_pos = map.miracles[my_camp].pos;
        enemy_pos = map.miracles[my_camp ^ 1].pos;
        //设定目标驻扎点为最近的驻扎点

        target_barrack = map.barracks[0].pos;
        //确定离自己基地最近的驻扎点的位置
        enemy_defense_pos1 = posShift(miracle_pos, "FF", 9);
        enemy_defense_pos1 = posShift(miracle_pos, "FF", 10);
        my_attack_pos1 = posShift(miracle_pos, "FF", 13);
        my_attack_pos2 = posShift(posShift(miracle_pos, "FF", 13), "SF", 1);
        my_attack_pos3 = posShift(posShift(miracle_pos, "FF", 13), "IF", 1);

        for (const auto &barrack : map.barracks)
        {
            if (cube_distance(miracle_pos, barrack.pos) <
                cube_distance(miracle_pos, target_barrack))
                target_barrack = barrack.pos;
        }

        // 在正中心偏右召唤一个弓箭手，用来抢占驻扎点
        summon("Archer", 1, posShift(miracle_pos, "SF", 1));
    }
    else if (round / 2 == 1)
    {
        auto ally_list = getUnitsByCamp(my_camp);
        id_barracks_sol = ally_list[0].id;
        move(id_barracks_sol, posShift(miracle_pos, "SF", 4));
        if (players[my_camp].mana >= 4)
        {
            summon("Archer", 2, posShift(miracle_pos, "SF", 1));
        }
    }
    else if (round / 2 == 2)
    {
        move(id_barracks_sol, target_barrack);
        auto ally_list = getUnitsByCamp(my_camp);
        if (ally_list.size() == 1)
        {
            if (players[my_camp].mana >= 4)
            {
                summon("Archer", 2, posShift(miracle_pos, "SF", 1));
            }
        }
        else
        {
            move(ally_list[1].id, posShift(posShift(miracle_pos, "SF", 1), "FF", 2));
        }
    }
    else
    {

        waitting_allay_list.clear();
        auto allay_list = getUnitsByCamp(my_camp);
        auto enemy_list = getUnitsByCamp(my_camp ^ 1);
        fprintf(stderr, "allay_list size:%d\n", allay_list.size());
        fprintf(stderr, "enemy_list size:%d\n", enemy_list.size());
        //构建地图没各位早位置的最大伤害
        auto all_pos = all_pos_in_map();
        std::vector<pair<Pos, int>> maxdamage_all_pos;
        int size_all_pose = all_pos.size();
        //===================mode_check=======================
        int danger = 0;
        for (auto enemy : enemy_list)
        {
            if (cube_distance(enemy.pos, miracle_pos) <= 4)
                danger++;
        }
        if (danger >= 3)
        {
            attack_mode = DEFENSE;
            fprintf(stderr, "mode DEFENSE\n");
        }
        else
        {
            attack_mode = ATTACK;
            fprintf(stderr, "mode ATTACK\n");
        }
        //===================================================
        //===================使用神器==========================
        //攻击状态多给蝙蝠，否则全给弓箭手
        if (attack_mode == ATTACK && players[my_camp].mana >= 6 && players[my_camp].artifact[0].state == "Ready")
        {
            static default_random_engine g(static_cast<unsigned int>(time(nullptr)));
            int randnum = uniform_int_distribution<>(0, 10)(g);
            fprintf(stderr, "sield %d\n", randnum);
            if (randnum < 3)
            {
                for (auto allay : allay_list)
                {
                    if (allay.type == "BlackBat" && allay.holy_shield == false)
                    {
                        use(players[my_camp].artifact[0].id, allay.id);
                    }
                }
            }
            else if (randnum < 5)
            {
                for (auto allay : allay_list)
                {
                    if (allay.type == "Archer" && allay.holy_shield == false)
                    {
                        use(players[my_camp].artifact[0].id, allay.id);
                    }
                }
            }
        }
        else if (attack_mode == DEFENSE && players[my_camp].mana >= 6 && players[my_camp].artifact[0].state == "Ready")
        {
            static default_random_engine g(static_cast<unsigned int>(time(nullptr)));
            int randnum = uniform_int_distribution<>(0, 10)(g);
            if (randnum < 5)
            {
                for (auto allay : allay_list)
                {
                    if (allay.type == "Archer" && allay.holy_shield == false)
                    {
                        use(players[my_camp].artifact[0].id, allay.id);
                    }
                }
            }
        }
        //=====================================================
        for (int i = 0; i < size_all_pose; i++)
        {
            int maxdamage = 0;
            for (auto enemy : enemy_list)
            {
                int dis = cube_distance(enemy.pos, all_pos[i]);
                if (enemy.atk_range[0] <= dis && enemy.atk_range[1] >= dis)
                {
                    if (enemy.atk > maxdamage)
                        maxdamage = enemy.atk;
                }
            }
            maxdamage_all_pos.push_back(make_pair(all_pos[i], maxdamage));
        }
        //为了防止下面while不明情况超时
        maxdamage_all_pos.push_back(make_pair(all_pos[0], 0));
        for (auto allay : allay_list)
        {
            int attack_id = -1;
            int flag = -1;
            if (allay.can_atk)
            {
                for (auto enemy : enemy_list)
                {
                    if (canAttack(allay, enemy))
                    { //第一优先级：可以杀死敌方单位且自身不会致命
                        if (allay.atk >= enemy.hp)
                        {
                            if (canAttack(enemy, allay))
                            {
                                if (enemy.atk < allay.hp)
                                {
                                    attack_id = enemy.id;
                                    flag = 1;
                                    fprintf(stderr, "%s first attack %s at %d %d %d\n", allay.type.c_str(), enemy.type.c_str(), get<0>(enemy.pos), get<1>(enemy.pos), get<2>(enemy.pos));
                                    break;
                                }
                            }
                            else
                            {
                                attack_id = enemy.id;
                                flag = 1;
                                fprintf(stderr, "%s first attack %s at %d %d %d\n", allay.type.c_str(), enemy.type.c_str(), get<0>(enemy.pos), get<1>(enemy.pos), get<2>(enemy.pos));
                                break;
                            }
                        }
                    }
                }
                if (flag == -1)
                {
                    //第二优先级打基地
                    int distance_to_enemy_camp = cube_distance(enemy_pos, allay.pos);
                    if (distance_to_enemy_camp >= allay.atk_range[0] && distance_to_enemy_camp <= allay.atk_range[1])
                    {
                        flag = 1;
                        attack_id = my_camp ^ 1;
                        enemy_miracle_hp -= allay.atk;
                        fprintf(stderr, "%s enemy_pos ,rest_hp=%d\n", allay.type.c_str(), enemy_miracle_hp);
                    }
                }
                if (flag == -1)
                {
                    for (auto enemy : enemy_list)
                    {
                        if (canAttack(allay, enemy))
                        { //第3优先级：可以攻击敌方单位且自身不会致命

                            if (canAttack(enemy, allay))
                            {
                                if (enemy.atk < allay.hp)
                                {
                                    attack_id = enemy.id;
                                    flag = 1;
                                    fprintf(stderr, "%s third attack %s at %d %d %d\n", allay.type.c_str(), enemy.type.c_str(), get<0>(enemy.pos), get<1>(enemy.pos), get<2>(enemy.pos));
                                    break;
                                }
                            }
                            else
                            {
                                attack_id = enemy.id;
                                flag = 1;
                                fprintf(stderr, "%s third attack %s at %d %d %d\n", allay.type.c_str(), enemy.type.c_str(), get<0>(enemy.pos), get<1>(enemy.pos), get<2>(enemy.pos));
                                break;
                            }
                        }
                    }
                }
                if (flag == -1)
                {
                    waitting_allay_list.push_back(allay);
                    fprintf(stderr, "%s waitting\n", allay.type.c_str());
                }
                else
                {
                    attack(allay.id, attack_id);
                }
            }
            for (auto allay : waitting_allay_list)
            {
                int index = 0;
                while ((all_pos[index] != allay.pos) && (index < all_pos.size() - 1))
                    index++;
                if (maxdamage_all_pos[index].second >= allay.hp)
                { //处于致命位置
                    fprintf(stderr, "%s criual\n", allay.type.c_str());
                    if (allay.type == "Archer") //"Archer", "BlackBat", "VolcanoDragon"
                    {

                        Pos target_pos;
                        //获取所有可到达的位置
                        auto reach_pos_with_dis = reachable(allay, map);
                        //压平
                        vector<Pos> reach_pos_list;
                        vector<Pos> unkilled_pos_list;
                        for (const auto &reach_pos : reach_pos_with_dis)
                        {
                            for (auto pos : reach_pos)
                                reach_pos_list.push_back(pos);
                        }
                        //计算不会被杀死的点
                        for (auto poss : reach_pos_list)
                        {
                            for (auto damage_pos : maxdamage_all_pos)
                            {
                                if (damage_pos.first == poss)
                                {
                                    if (damage_pos.second < allay.hp)
                                    {
                                        unkilled_pos_list.push_back(poss);
                                    }
                                    break;
                                }
                            }
                        }
                        //计算能攻击到敌方最多的点
                        std::vector<pair<Pos, int>> max_unit_can_attack_all_pos;
                        for (auto poss : unkilled_pos_list)
                        {
                            int can_attack = 0;
                            for (auto enemy : enemy_list)
                            {
                                int dis = cube_distance(poss, enemy.pos);
                                if (dis >= allay.atk_range[0] && dis <= allay.atk_range[1])
                                {
                                    can_attack++;
                                }
                            }
                            max_unit_can_attack_all_pos.push_back(make_pair(poss, can_attack));
                        }

                        sort(max_unit_can_attack_all_pos.begin(), max_unit_can_attack_all_pos.end(),
                             [&](const pair<Pos, int> &max_unit_can_attack_all_pos1, const pair<Pos, int> &max_unit_can_attack_all_pos2) { return max_unit_can_attack_all_pos1.second > max_unit_can_attack_all_pos2.second; });
                        if (!max_unit_can_attack_all_pos.empty() && allay.pos != target_barrack)
                        {
                            move(allay.id, max_unit_can_attack_all_pos[0].first);
                            fprintf(stderr, "%s   move to %d %d %d\n", allay.type.c_str(), get<0>(max_unit_can_attack_all_pos[0].first), get<1>(max_unit_can_attack_all_pos[0].first), get<2>(max_unit_can_attack_all_pos[0].first));
                        }
                    }
                    else if (allay.type == "VolcanoDragon")
                    {
                        auto reach_pos_with_dis = reachable(allay, map);
                        //压平
                        vector<Pos> reach_pos_list;
                        vector<Pos> attackable_pos_list;
                        for (const auto &reach_pos : reach_pos_with_dis)
                        {
                            for (auto pos : reach_pos)
                                reach_pos_list.push_back(pos);
                        }
                        //计算能攻击到地面敌人的点
                        for (auto poss : reach_pos_list)
                        {
                            for (auto enemy : enemy_list)
                            {
                                int dis = cube_distance(enemy.pos, poss);
                                if ((dis >= 1 && dis <= 2) && (enemy.flying == false))
                                {
                                    attackable_pos_list.push_back(poss);
                                    break;
                                }
                            }
                        }
                        //随机选一个
                        static default_random_engine g(static_cast<unsigned int>(time(nullptr)));
                        int randnum = uniform_int_distribution<>(0, attackable_pos_list.size() - 1)(g);
                        if (!attackable_pos_list.empty())
                        {
                            move(allay.id, attackable_pos_list[randnum]);
                            fprintf(stderr, "%s   move to %d %d %d\n", allay.type.c_str(), get<0>(attackable_pos_list[randnum]), get<1>(attackable_pos_list[randnum]), get<2>(attackable_pos_list[randnum]));
                        }
                    }
                    else if (allay.type == "BlackBat")
                    {
                        auto reach_pos_with_dis = reachable(allay, map);
                        //压平
                        vector<Pos> reach_pos_list;
                        vector<Pos> unkilled_pos_list;
                        vector<Pos> attack_jidi_pos_list;
                        for (const auto &reach_pos : reach_pos_with_dis)
                        {
                            for (auto pos : reach_pos)
                                reach_pos_list.push_back(pos);
                        }
                        //计算不会被杀死的点
                        bool attack_archer = false;
                        Pos archer_pos;
                        for (auto poss : reach_pos_list)
                        {
                            bool safe = true;
                            for (auto enemy : enemy_list)
                            {
                                if (enemy.atk_flying == true)
                                {
                                    int dis = cube_distance(poss, enemy.pos);
                                    if ((dis >= enemy.atk_range[0] && dis <= enemy.atk_range[1]) && (allay.hp <= enemy.atk))
                                    {
                                        safe = false;
                                        break;
                                    }
                                }
                            }
                            if (safe == true)
                            {
                                if (cube_distance(poss, enemy_pos) == 1)
                                {
                                    attack_jidi_pos_list.push_back(poss);
                                }
                                else
                                {
                                    unkilled_pos_list.push_back(poss);
                                }
                            }
                        }
                        sort(unkilled_pos_list.begin(), unkilled_pos_list.end(),
                             [&](const Pos &unkilled_pos_list1, const Pos &unkilled_pos_list2) { return cube_distance(unkilled_pos_list1, enemy_pos) < cube_distance(unkilled_pos_list2, enemy_pos); });
                        static default_random_engine kk(static_cast<unsigned int>(time(nullptr)));
                        int randnumkk = uniform_int_distribution<>(0, 10)(kk);
                        fprintf(stderr, "randnumkk:%d\n", randnumkk);
                        if (!attack_jidi_pos_list.empty())
                        {
                            static default_random_engine g(static_cast<unsigned int>(time(nullptr)));
                            int randnum = uniform_int_distribution<>(0, attack_jidi_pos_list.size() - 1)(g);
                            fprintf(stderr, "%s   move to %d %d %d\n", allay.type.c_str(), get<0>(attack_jidi_pos_list[randnum]), get<1>(attack_jidi_pos_list[randnum]), get<2>(attack_jidi_pos_list[randnum]));
                            move(allay.id, attack_jidi_pos_list[randnum]);
                        }
                        //百分之七十出现勇敢属性，冲基地
                        else if (randnumkk < 7)
                        {
                            fprintf(stderr, "tests\n");
                            int flag = -1;
                            for (auto poss : reach_pos_list)
                            {
                                if (poss == my_attack_pos2)
                                {
                                    flag = 1;
                                    move(allay.id, my_attack_pos2);
                                    fprintf(stderr, "brave %s   move to %d %d %d\n", allay.type.c_str(), get<0>(my_attack_pos2), get<1>(my_attack_pos2), get<2>(my_attack_pos2));
                                }
                            }
                            if (flag == -1)
                            {
                                for (auto poss : reach_pos_list)
                                {
                                    if (poss == my_attack_pos3)
                                    {
                                        flag = 1;
                                        move(allay.id, my_attack_pos3);
                                        fprintf(stderr, "brave %s   move to %d %d %d\n", allay.type.c_str(), get<0>(my_attack_pos3), get<1>(my_attack_pos3), get<2>(my_attack_pos3));
                                    }
                                }
                            }
                            if (flag == -1)
                            {
                                for (auto poss : reach_pos_list)
                                {
                                    if (poss == my_attack_pos1)
                                    {
                                        flag = 1;
                                        move(allay.id, my_attack_pos1);
                                        fprintf(stderr, "brave %s   move to %d %d %d\n", allay.type.c_str(), get<0>(my_attack_pos1), get<1>(my_attack_pos1), get<2>(my_attack_pos1));
                                    }
                                }
                            }
                            if (flag == -1)
                            {
                                if (!unkilled_pos_list.empty())
                                {
                                    move(allay.id, unkilled_pos_list[0]);
                                    fprintf(stderr, "%s   move to %d %d %d\n", allay.type.c_str(), get<0>(reach_pos_list[0]), get<1>(reach_pos_list[0]), get<2>(reach_pos_list[0]));
                                }
                            }
                        }
                    }
                }
                else //处于非致命位置
                {
                    fprintf(stderr, "%s   no crauial\n", allay.type.c_str());

                    if (allay.type == "Archer") //"Archer", "BlackBat", "VolcanoDragon"
                    {
                        Pos target_pos;
                        //获取所有可到达的位置
                        auto reach_pos_with_dis = reachable(allay, map);
                        //压平
                        vector<Pos> reach_pos_list;
                        vector<Pos> unkilled_pos_list;
                        for (const auto &reach_pos : reach_pos_with_dis)
                        {
                            for (auto pos : reach_pos)
                                reach_pos_list.push_back(pos);
                        }
                        //计算不会被杀死的点
                        for (auto poss : reach_pos_list)
                        {
                            for (auto damage_pos : maxdamage_all_pos)
                            {
                                if (damage_pos.first == poss)
                                {
                                    if (damage_pos.second < allay.hp)
                                    {
                                        unkilled_pos_list.push_back(poss);
                                    }
                                    break;
                                }
                            }
                        }
                        //计算能攻击到敌方最多的点
                        std::vector<pair<Pos, int>> max_unit_can_attack_all_pos;
                        for (auto poss : unkilled_pos_list)
                        {
                            int can_attack = 0;
                            for (auto enemy : enemy_list)
                            {
                                int dis = cube_distance(poss, enemy.pos);
                                if (dis >= allay.atk_range[0] && dis <= allay.atk_range[1])
                                {
                                    can_attack++;
                                }
                            }
                            max_unit_can_attack_all_pos.push_back(make_pair(poss, can_attack));
                        }

                        sort(max_unit_can_attack_all_pos.begin(), max_unit_can_attack_all_pos.end(),
                             [&](const pair<Pos, int> &max_unit_can_attack_all_pos1, const pair<Pos, int> &max_unit_can_attack_all_pos2) { return max_unit_can_attack_all_pos1.second > max_unit_can_attack_all_pos2.second; });
                        if (!max_unit_can_attack_all_pos.empty() && allay.pos != target_barrack)
                        {
                            move(allay.id, max_unit_can_attack_all_pos[0].first);
                            fprintf(stderr, "%s   move to %d %d %d\n", allay.type.c_str(), get<0>(max_unit_can_attack_all_pos[0].first), get<1>(max_unit_can_attack_all_pos[0].first), get<2>(max_unit_can_attack_all_pos[0].first));
                        }
                    }
                    else if (allay.type == "BlackBat")
                    {
                        auto reach_pos_with_dis = reachable(allay, map);
                        //压平
                        vector<Pos> reach_pos_list;
                        vector<Pos> unkilled_pos_list;
                        vector<Pos> attack_jidi_pos_list;
                        for (const auto &reach_pos : reach_pos_with_dis)
                        {
                            for (auto pos : reach_pos)
                                reach_pos_list.push_back(pos);
                        }
                        //计算不会被杀死的点
                        bool attack_archer = false;
                        Pos archer_pos;
                        for (auto poss : reach_pos_list)
                        {
                            bool safe = true;
                            for (auto enemy : enemy_list)
                            {
                                if (enemy.atk_flying == true)
                                {
                                    int dis = cube_distance(poss, enemy.pos);
                                    if ((dis >= enemy.atk_range[0] && dis <= enemy.atk_range[1]) && (allay.hp <= enemy.atk))
                                    {
                                        safe = false;
                                        break;
                                    }
                                }
                            }
                            if (safe == true)
                            {
                                if (cube_distance(poss, enemy_pos) == 1)
                                {
                                    attack_jidi_pos_list.push_back(poss);
                                }
                                else
                                {
                                    unkilled_pos_list.push_back(poss);
                                }
                            }
                        }
                        sort(unkilled_pos_list.begin(), unkilled_pos_list.end(),
                             [&](const Pos &unkilled_pos_list1, const Pos &unkilled_pos_list2) { return cube_distance(unkilled_pos_list1, enemy_pos) < cube_distance(unkilled_pos_list2, enemy_pos); });
                        static default_random_engine kk(static_cast<unsigned int>(time(nullptr)));
                        int randnumkk = uniform_int_distribution<>(0, 10)(kk);
                        fprintf(stderr, "randnumkk:%d\n", randnumkk);
                        if (!attack_jidi_pos_list.empty())
                        {
                            static default_random_engine g(static_cast<unsigned int>(time(nullptr)));
                            int randnum = uniform_int_distribution<>(0, attack_jidi_pos_list.size() - 1)(g);
                            move(allay.id, attack_jidi_pos_list[randnum]);
                            fprintf(stderr, "%s   move to %d %d %d\n", allay.type.c_str(), get<0>(attack_jidi_pos_list[randnum]), get<1>(attack_jidi_pos_list[randnum]), get<2>(attack_jidi_pos_list[randnum]));
                        }
                        else if (randnumkk < 7)
                        {
                            //百分之七十出现勇敢属性，冲基地
                            fprintf(stderr, "tests\n");
                            int flag = -1;
                            for (auto poss : reach_pos_list)
                            {
                                if (poss == my_attack_pos2)
                                {
                                    flag = 1;
                                    move(allay.id, my_attack_pos2);
                                    fprintf(stderr, "brave %s   move to %d %d %d\n", allay.type.c_str(), get<0>(my_attack_pos2), get<1>(my_attack_pos2), get<2>(my_attack_pos2));
                                }
                            }
                            if (flag == -1)
                            {
                                for (auto poss : reach_pos_list)
                                {
                                    if (poss == my_attack_pos3)
                                    {
                                        flag = 1;
                                        move(allay.id, my_attack_pos3);
                                        fprintf(stderr, "brave %s   move to %d %d %d\n", allay.type.c_str(), get<0>(my_attack_pos3), get<1>(my_attack_pos3), get<2>(my_attack_pos3));
                                    }
                                }
                            }
                            if (flag == -1)
                            {
                                for (auto poss : reach_pos_list)
                                {
                                    if (poss == my_attack_pos1)
                                    {
                                        flag = 1;
                                        move(allay.id, my_attack_pos1);
                                        fprintf(stderr, "brave %s   move to %d %d %d\n", allay.type.c_str(), get<0>(my_attack_pos1), get<1>(my_attack_pos1), get<2>(my_attack_pos1));
                                    }
                                }
                            }
                            if (flag == -1)
                            {
                                if (!unkilled_pos_list.empty())
                                {
                                    move(allay.id, unkilled_pos_list[0]);
                                    fprintf(stderr, "%s   move to %d %d %d\n", allay.type.c_str(), get<0>(reach_pos_list[0]), get<1>(reach_pos_list[0]), get<2>(reach_pos_list[0]));
                                }
                            }
                        }
                    }
                    else if (allay.type == "VolcanoDragon")
                    {
                        //donothing
                    }
                }
            }

            if (attack_mode == ATTACK)
            {
                static default_random_engine g(static_cast<unsigned int>(time(nullptr)));
                auto summon_pos_list = getSummonPosByCamp(my_camp);
                vector<Pos> available_summon_pos_list;
                for (auto pos : summon_pos_list)
                {
                    auto unit_on_pos_ground = getUnitByPos(pos, false);
                    if (unit_on_pos_ground.id == -1)
                        available_summon_pos_list.push_back(pos);
                }
                //统计距离自己家最近和敌人家最近的两个列表
                auto near_enemy_pos_list = available_summon_pos_list;
                auto near_miracle_pos_list = available_summon_pos_list;
                int near_used = 0;
                int far_used = 0;
                sort(near_enemy_pos_list.begin(), near_enemy_pos_list.end(), [this](Pos _pos1, Pos _pos2) {
                    return cube_distance(_pos1, enemy_pos) < cube_distance(_pos2, enemy_pos);
                });
                sort(near_miracle_pos_list.begin(), near_miracle_pos_list.end(), [this](Pos _pos1, Pos _pos2) {
                    return cube_distance(_pos1, miracle_pos) < cube_distance(_pos2, miracle_pos);
                });
                //统计各个生物的可用数量
                int mana = players[my_camp].mana;
                auto deck = players[my_camp].creature_capacity;
                ::map<string, int> available_count;
                for (const auto &card_unit : deck)
                    available_count[card_unit.type] = card_unit.available_count;
                //如果可以生成弓箭手则生成
                if (available_count["Archer"] > 0)
                {
                    if (mana >= CARD_DICT.at("Archer")[3].cost)
                    {
                        mana -= CARD_DICT.at("Archer")[3].cost;
                        summon("Archer", 3, near_miracle_pos_list[0]);
                        available_count["Archer"] -= 1;
                    }
                }
                int randmod3 = uniform_int_distribution<>(0, 3)(g);
                fprintf(stderr, "randmod3 : %d\n", randmod3);
                //随机生成蝙蝠或者龙3：1
                if (randmod3 < 3)
                { //生成三级蝙蝠
                    if (available_count["BlackBat"] > 0 && mana >= CARD_DICT.at("BlackBat")[3].cost)
                    {
                        fprintf(stderr, "generate BlackBat\n");
                        mana -= CARD_DICT.at("BlackBat")[3].cost;
                        summon("BlackBat", 3, near_enemy_pos_list[0]);
                        available_count["BlackBat"] -= 1;
                        available_summon_pos_list.clear();
                        for (auto pos : summon_pos_list)
                        {
                            auto unit_on_pos_ground = getUnitByPos(pos, false);
                            if (unit_on_pos_ground.id == -1)
                                available_summon_pos_list.push_back(pos);
                        }
                        near_enemy_pos_list = available_summon_pos_list;
                        near_miracle_pos_list = available_summon_pos_list;
                        sort(near_enemy_pos_list.begin(), near_enemy_pos_list.end(), [this](Pos _pos1, Pos _pos2) {
                            return cube_distance(_pos1, enemy_pos) < cube_distance(_pos2, enemy_pos);
                        });
                        sort(near_miracle_pos_list.begin(), near_miracle_pos_list.end(), [this](Pos _pos1, Pos _pos2) {
                            return cube_distance(_pos1, miracle_pos) < cube_distance(_pos2, miracle_pos);
                        });
                    }
                    if (available_count["BlackBat"] > 0 && mana >= CARD_DICT.at("BlackBat")[3].cost)
                    {
                        fprintf(stderr, "generate BlackBat\n");
                        mana -= CARD_DICT.at("BlackBat")[3].cost;
                        summon("BlackBat", 3, near_enemy_pos_list[0]);
                        available_count["BlackBat"] -= 1;
                        available_summon_pos_list.clear();
                        for (auto pos : summon_pos_list)
                        {
                            auto unit_on_pos_ground = getUnitByPos(pos, false);
                            if (unit_on_pos_ground.id == -1)
                                available_summon_pos_list.push_back(pos);
                        }
                        near_enemy_pos_list = available_summon_pos_list;
                        near_miracle_pos_list = available_summon_pos_list;
                        sort(near_enemy_pos_list.begin(), near_enemy_pos_list.end(), [this](Pos _pos1, Pos _pos2) {
                            return cube_distance(_pos1, enemy_pos) < cube_distance(_pos2, enemy_pos);
                        });
                        sort(near_miracle_pos_list.begin(), near_miracle_pos_list.end(), [this](Pos _pos1, Pos _pos2) {
                            return cube_distance(_pos1, miracle_pos) < cube_distance(_pos2, miracle_pos);
                        });
                    }
                }
                else
                {
                    if (available_count["VolcanoDragon"] > 0 && mana >= CARD_DICT.at("VolcanoDragon")[3].cost)
                    {
                        fprintf(stderr, "generate VolcanoDragon\n");

                        mana -= CARD_DICT.at("VolcanoDragon")[3].cost;
                        summon("VolcanoDragon", 3, near_miracle_pos_list[0]);
                        available_count["VolcanoDragon"] -= 1;
                        //重新计算summon——pos
                        available_summon_pos_list.clear();
                        for (auto pos : summon_pos_list)
                        {
                            auto unit_on_pos_ground = getUnitByPos(pos, false);
                            if (unit_on_pos_ground.id == -1)
                                available_summon_pos_list.push_back(pos);
                        }
                        near_enemy_pos_list = available_summon_pos_list;
                        near_miracle_pos_list = available_summon_pos_list;
                        sort(near_enemy_pos_list.begin(), near_enemy_pos_list.end(), [this](Pos _pos1, Pos _pos2) {
                            return cube_distance(_pos1, enemy_pos) < cube_distance(_pos2, enemy_pos);
                        });
                        sort(near_miracle_pos_list.begin(), near_miracle_pos_list.end(), [this](Pos _pos1, Pos _pos2) {
                            return cube_distance(_pos1, miracle_pos) < cube_distance(_pos2, miracle_pos);
                        });
                    }
                }
            }
            else if (attack_mode == DEFENSE)
            {
                static default_random_engine g(static_cast<unsigned int>(time(nullptr)));
                auto summon_pos_list = getSummonPosByCamp(my_camp);
                vector<Pos> available_summon_pos_list;
                for (auto pos : summon_pos_list)
                {
                    auto unit_on_pos_ground = getUnitByPos(pos, false);
                    if (unit_on_pos_ground.id == -1)
                        available_summon_pos_list.push_back(pos);
                }
                //统计距离自己家最近和敌人家最近的两个列表
                auto near_enemy_pos_list = available_summon_pos_list;
                auto near_miracle_pos_list = available_summon_pos_list;
                int near_used = 0;
                int far_used = 0;
                sort(near_enemy_pos_list.begin(), near_enemy_pos_list.end(), [this](Pos _pos1, Pos _pos2) {
                    return cube_distance(_pos1, enemy_pos) < cube_distance(_pos2, enemy_pos);
                });
                sort(near_miracle_pos_list.begin(), near_miracle_pos_list.end(), [this](Pos _pos1, Pos _pos2) {
                    return cube_distance(_pos1, miracle_pos) < cube_distance(_pos2, miracle_pos);
                });
                //统计各个生物的可用数量
                int mana = players[my_camp].mana;
                auto deck = players[my_camp].creature_capacity;
                ::map<string, int> available_count;
                for (const auto &card_unit : deck)
                    available_count[card_unit.type] = card_unit.available_count;
                //如果可以生成弓箭手则生成
                if (available_count["Archer"] > 0)
                {
                    if (mana >= CARD_DICT.at("Archer")[3].cost)
                    {
                        mana -= CARD_DICT.at("Archer")[3].cost;
                        summon("Archer", 3, near_miracle_pos_list[0]);
                        available_count["Archer"] -= 1;
                    }
                }
                available_summon_pos_list.clear();
                for (auto pos : summon_pos_list)
                {
                    auto unit_on_pos_ground = getUnitByPos(pos, false);
                    if (unit_on_pos_ground.id == -1)
                        available_summon_pos_list.push_back(pos);
                }
                near_enemy_pos_list = available_summon_pos_list;
                near_miracle_pos_list = available_summon_pos_list;
                sort(near_enemy_pos_list.begin(), near_enemy_pos_list.end(), [this](Pos _pos1, Pos _pos2) {
                    return cube_distance(_pos1, enemy_pos) < cube_distance(_pos2, enemy_pos);
                });
                sort(near_miracle_pos_list.begin(), near_miracle_pos_list.end(), [this](Pos _pos1, Pos _pos2) {
                    return cube_distance(_pos1, miracle_pos) < cube_distance(_pos2, miracle_pos);
                });
                //如果可以生成弓箭手则生成
                if (available_count["Archer"] > 0)
                {
                    if (mana >= CARD_DICT.at("Archer")[3].cost)
                    {
                        fprintf(stderr, "generate Archer\n");

                        mana -= CARD_DICT.at("Archer")[3].cost;
                        summon("Archer", 3, near_miracle_pos_list[0]);
                        available_count["Archer"] -= 1;
                    }
                }

                int randmod3 = uniform_int_distribution<>(0, 3)(g);
                fprintf(stderr, "randmod3 : %d\n", randmod3);
                if (randmod3 <= 2)
                { //生成三级蝙蝠

                    if (available_count["BlackBat"] > 0 && mana >= CARD_DICT.at("BlackBat")[3].cost)
                    {
                        fprintf(stderr, "generate BlackBat\n");

                        mana -= CARD_DICT.at("BlackBat")[3].cost;
                        summon("BlackBat", 3, near_enemy_pos_list[0]);
                        available_count["BlackBat"] -= 1;
                        available_summon_pos_list.clear();
                        for (auto pos : summon_pos_list)
                        {
                            auto unit_on_pos_ground = getUnitByPos(pos, false);
                            if (unit_on_pos_ground.id == -1)
                                available_summon_pos_list.push_back(pos);
                        }
                        near_enemy_pos_list = available_summon_pos_list;
                        near_miracle_pos_list = available_summon_pos_list;
                        sort(near_enemy_pos_list.begin(), near_enemy_pos_list.end(), [this](Pos _pos1, Pos _pos2) {
                            return cube_distance(_pos1, enemy_pos) < cube_distance(_pos2, enemy_pos);
                        });
                        sort(near_miracle_pos_list.begin(), near_miracle_pos_list.end(), [this](Pos _pos1, Pos _pos2) {
                            return cube_distance(_pos1, miracle_pos) < cube_distance(_pos2, miracle_pos);
                        });
                    }
                    if (available_count["BlackBat"] > 0 && mana >= CARD_DICT.at("BlackBat")[3].cost)
                    {
                        fprintf(stderr, "generate BlackBat\n");

                        mana -= CARD_DICT.at("BlackBat")[3].cost;
                        summon("BlackBat", 3, near_enemy_pos_list[0]);
                        available_count["BlackBat"] -= 1;
                        available_summon_pos_list.clear();
                        for (auto pos : summon_pos_list)
                        {
                            auto unit_on_pos_ground = getUnitByPos(pos, false);
                            if (unit_on_pos_ground.id == -1)
                                available_summon_pos_list.push_back(pos);
                        }
                        near_enemy_pos_list = available_summon_pos_list;
                        near_miracle_pos_list = available_summon_pos_list;
                        sort(near_enemy_pos_list.begin(), near_enemy_pos_list.end(), [this](Pos _pos1, Pos _pos2) {
                            return cube_distance(_pos1, enemy_pos) < cube_distance(_pos2, enemy_pos);
                        });
                        sort(near_miracle_pos_list.begin(), near_miracle_pos_list.end(), [this](Pos _pos1, Pos _pos2) {
                            return cube_distance(_pos1, miracle_pos) < cube_distance(_pos2, miracle_pos);
                        });
                    }
                }
                else
                {
                    if (available_count["VolcanoDragon"] > 0 && mana >= CARD_DICT.at("VolcanoDragon")[3].cost)
                    {
                        fprintf(stderr, "generate VolcanoDragon\n");

                        mana -= CARD_DICT.at("VolcanoDragon")[3].cost;
                        summon("VolcanoDragon", 3, near_miracle_pos_list[0]);
                        available_count["VolcanoDragon"] -= 1;

                        //重新计算summon——pos
                        available_summon_pos_list.clear();
                        for (auto pos : summon_pos_list)
                        {
                            auto unit_on_pos_ground = getUnitByPos(pos, false);
                            if (unit_on_pos_ground.id == -1)
                                available_summon_pos_list.push_back(pos);
                        }
                        near_enemy_pos_list = available_summon_pos_list;
                        near_miracle_pos_list = available_summon_pos_list;
                        sort(near_enemy_pos_list.begin(), near_enemy_pos_list.end(), [this](Pos _pos1, Pos _pos2) {
                            return cube_distance(_pos1, enemy_pos) < cube_distance(_pos2, enemy_pos);
                        });
                        sort(near_miracle_pos_list.begin(), near_miracle_pos_list.end(), [this](Pos _pos1, Pos _pos2) {
                            return cube_distance(_pos1, miracle_pos) < cube_distance(_pos2, miracle_pos);
                        });
                    }
                }
            }
        }
        //===================使用神器==========================
        //攻击状态多给蝙蝠，否则全给弓箭手
        if (attack_mode == ATTACK && players[my_camp].mana >= 6 && players[my_camp].artifact[0].state == "Ready")
        {
            static default_random_engine g(static_cast<unsigned int>(time(nullptr)));
            int randnum = uniform_int_distribution<>(0, 10)(g);
            fprintf(stderr, "sield %d\n", randnum);
            if (randnum < 6)
            {
                for (auto allay : allay_list)
                {
                    if ((allay.type == "BlackBat") && allay.holy_shield == false)
                    {
                        fprintf(stderr, "give sieid to BlackBat\n");
                        use(players[my_camp].artifact[0].id, allay.id);
                        break;
                    }
                }
            }
            else if (randnum < 8)
            {
                for (auto allay : allay_list)
                {
                    if ((allay.type == "Archer") && allay.holy_shield == false)
                    {
                        fprintf(stderr, "give sieid to Archer\n");
                        use(players[my_camp].artifact[0].id, allay.id);
                        break;
                    }
                }
            }
        }
        else if (attack_mode == DEFENSE && players[my_camp].mana >= 6 && players[my_camp].artifact[0].state == "Ready")
        {
            static default_random_engine g(static_cast<unsigned int>(time(nullptr)));
            int randnum = uniform_int_distribution<>(0, 10)(g);
            if (randnum < 8)
            {
                for (auto allay : allay_list)
                {
                    if ((allay.type == "Archer") && allay.holy_shield == false)
                    {
                        fprintf(stderr, "give sieid to Archer\n");
                        use(players[my_camp].artifact[0].id, allay.id);
                        break;
                    }
                }
            }
        }
        //=====================================================
    }
    endRound();
}

Pos AI::posShift(Pos pos, string direct, int shift_num)
{
    /*
     * 对于给定位置，给出按照自己的视角（神迹在最下方）的某个方向移动一步后的位置
     * 本段代码可以自由取用
     * @param pos:  (x, y, z)
     * @param direct: 一个str，含2个字符，意义见注释
     * @return: 移动后的位置 (x', y', z')
     */
    transform(direct.begin(), direct.end(), direct.begin(), ::toupper);
    if (my_camp == 0)
    {
        if (direct == "FF") //正前方
            return make_tuple(get<0>(pos) + shift_num, get<1>(pos) - shift_num, get<2>(pos));
        else if (direct == "SF") //优势路前方（自身视角右侧为优势路）
            return make_tuple(get<0>(pos) + shift_num, get<1>(pos), get<2>(pos) - shift_num);
        else if (direct == "IF") //劣势路前方
            return make_tuple(get<0>(pos), get<1>(pos) + shift_num, get<2>(pos) - shift_num);
        else if (direct == "BB") //正后方
            return make_tuple(get<0>(pos) - shift_num, get<1>(pos) + shift_num, get<2>(pos));
        else if (direct == "SB") //优势路后方
            return make_tuple(get<0>(pos), get<1>(pos) - shift_num, get<2>(pos) + shift_num);
        else if (direct == "IB") //劣势路后方
            return make_tuple(get<0>(pos) - shift_num, get<1>(pos), get<2>(pos) + shift_num);
    }
    else
    {
        if (direct == "FF") //正前方
            return make_tuple(get<0>(pos) - shift_num, get<1>(pos) + shift_num, get<2>(pos));
        else if (direct == "SF") //优势路前方（自身视角右侧为优势路）
            return make_tuple(get<0>(pos) - shift_num, get<1>(pos), get<2>(pos) + shift_num);
        else if (direct == "IF") //劣势路前方
            return make_tuple(get<0>(pos), get<1>(pos) - shift_num, get<2>(pos) + shift_num);
        else if (direct == "BB") //正后方
            return make_tuple(get<0>(pos) + shift_num, get<1>(pos) - shift_num, get<2>(pos));
        else if (direct == "SB") //优势路后方
            return make_tuple(get<0>(pos), get<1>(pos) + shift_num, get<2>(pos) - shift_num);
        else if (direct == "IB") //劣势路后方
            return make_tuple(get<0>(pos) + shift_num, get<1>(pos), get<2>(pos) - shift_num);
    }
}

int main()
{
    std::ifstream datafile("Data.json");
    json all_data;
    datafile >> all_data;
    card::get_data_from_json(all_data);

    AI player_ai;
    player_ai.chooseCards();
    while (true)
    {
        player_ai.updateGameInfo();
        player_ai.play();
    }
    return 0;
}
