#pragma once

#include "core/game.hpp"

namespace ttt::my_player {

using game::Event;
using game::IPlayer;
using game::Point;
using game::Sign;
using game::State;

class MyPlayer : public IPlayer {
  Sign m_sign = Sign::NONE;
  const char *m_name;  
  const double ATK_COEFF = 1.0;// Коэффициенты стратегии 
  const double DEF_COEFF = 1.1;// Коэффициенты стратегии 
  bool open_before = false;// линия открыта до. Заполняет check_line
  bool open_after = false;// линия открыта после. Заполняет check_line
  int check_line(const State &state,int &count, int start_x, int start_y, int dx, int dy, int win_len, bool is_attack);
public:
  MyPlayer(const char *name) : m_sign(Sign::NONE), m_name(name) {}
  void set_sign(Sign sign) override;
  Point make_move(const State &game) override;
  const char *get_name() const override;
};

class MyPlayer2 : public IPlayer {
  Sign m_sign = Sign::NONE;
  const char *m_name;
  int temp=1;

public:
  MyPlayer2(const char *name) : m_sign(Sign::NONE), m_name(name) {}
  void set_sign(Sign sign) override;
  Point make_move(const State &game) override;
  const char *get_name() const override;
};

}; // namespace ttt::my_player
