#include "my_player.hpp"
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <cmath>

namespace ttt::my_player {

void MyPlayer::set_sign(Sign sign) { m_sign = sign; }
const char *MyPlayer::get_name() const { return m_name; }

// Структура для хранения оценки каждой пустой клетки
struct RatedMovePoint {
    Point point;            // Координаты клетки
    double atk_weight;      // Aтакующий вес клетки
    double def_weight;      // Защитный вес клетки
    double weight;          // Итоговый вес: (atk_weight * ATK_COEFF) + (def_weight * DEF_COEFF)
    double distance;        // Расстояние от клетки до центра поля
};

// Функция для вычисления расстояния от точки до центра поля
double getDistanceToCenter(int x, int y, int rows, int cols) {
    double cx = (cols - 1) / 2.0;
    double cy = (rows - 1) / 2.0;
    return std::sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));
}

// Проверка линии и подсчёт её веса
int  MyPlayer::check_line(const State &state, int start_x, int start_y, int dx, int dy, int win_len, bool is_attack) {
    int rows = state.get_opts().rows;
    int cols = state.get_opts().cols;
    open_before = false;// линия открыта до
    open_after = false;// линия открытв после
    int count = 0;
    
    // Определяем знаки на основе поля m_sign
    Sign my_sign = m_sign;
    Sign opponent_sign = (m_sign == Sign::X) ? Sign::O : Sign::X;
    
    // Если считаем атаку — ищем свои, а чужие блокируют линию. Если защиту — наоборот.
    if(!is_attack){
      std::swap(my_sign,opponent_sign);
    }
    
    for (int i = 0; i < win_len; ++i) {
        int curr_x = start_x + i * dx;
        int curr_y = start_y + i * dy;
        
        Sign cell = state.get_value(curr_x, curr_y);
        if (cell == opponent_sign || cell == Sign::WALL) {
            return 0; 
        }
        if (cell == my_sign) {
            count++;
        }
    }
        
    // Проверяем линия открыта до
    int before_x = start_x - dx;
    int before_y = start_y - dy;
    if (before_x >= 0 && before_x < cols && before_y >= 0 && before_y < rows) {
        if (state.get_value(before_x, before_y) == Sign::NONE) open_before = true;
    }
    
    // Проверяем линия открыта после
    int after_x = start_x + win_len * dx;
    int after_y = start_y + win_len * dy;
    if (after_x >= 0 && after_x < cols && after_y >= 0 && after_y < rows) {
        if (state.get_value(after_x, after_y) == Sign::NONE) open_after = true;
    }
    if (count > 1){
        int weight = 1;
        for (int i=0; i < count; i++){
            weight*=10; 
        }
        if (open_before && open_after) weight *= 10; //*2 если открыта
        return weight;
    }
    else{
        return count*10;
    }

}

Point MyPlayer::make_move(const State &state) {
    int rows = state.get_opts().rows;
    int cols = state.get_opts().cols;
    int win_len = state.get_opts().win_len;    
    Sign opponent_sign = (m_sign == Sign::X) ? Sign::O : Sign::X;
    static const struct {
      int dx;
      int dy;
    } directions[] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
        
    std::vector<RatedMovePoint> immediate_win;       
    std::vector<RatedMovePoint> immediate_def;     
    std::vector<RatedMovePoint> immediate_open_four;           
    std::vector<RatedMovePoint> moves;     

    // 1) Расчет веса для пустых клеток.
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            if (state.get_value(x, y) != Sign::NONE) {
                continue;
            }
            bool can_immediate_win = false;
            bool can_immediate_open_four = false;
            bool must_immediate_def = false;

            RatedMovePoint move = { {x, y}, 0.0, 0.0, 0.0, getDistanceToCenter(x, y, rows, cols) };
            for (const auto dir : directions) {            
                int dx = dir.dx;//D_X[d];
                int dy = dir.dy;//D_Y[d];
                
                int atk_dir_weight = 0;
                int def_dir_weight = 0;

                for (int shift = 0; shift < win_len; shift++) {
                    int start_x = x - shift * dx;
                    int start_y = y - shift * dy;
                    int end_x = start_x + (win_len - 1) * dx;
                    int end_y = start_y + (win_len - 1) * dy;
                    
                    if (start_x < 0 || start_x >= cols || start_y < 0 || start_y >= rows ||
                        end_x < 0 || end_x >= cols || end_y < 0 || end_y >= rows) {
                        continue;
                    }
                    
                    // Анализ Атаки
                    int count_my = 0;
                    bool blocked_my = false;
                    for (int i = 0; i < win_len; ++i) {
                        Sign cell = state.get_value(start_x + i * dx, start_y + i * dy);
                        if (cell == opponent_sign || cell == Sign::WALL) { blocked_my = true; break; }
                        if (cell == m_sign) count_my++;
                    }
                    
                    if (!blocked_my && count_my > 0) {
                        if (count_my == win_len - 1) { can_immediate_win = true; }
                        atk_dir_weight = check_line(state, start_x, start_y, dx, dy, win_len, true);
                        if (count_my == win_len - 2) {
                            if (open_before && open_after) { can_immediate_open_four = true; }
                        }                        
                    }
                    
                    // Анализ Защиты
                    int count_op = 0;
                    bool blocked_op = false;
                    for (int i = 0; i < win_len; ++i) {
                        Sign cell = state.get_value(start_x + i * dx, start_y + i * dy);
                        if (cell == m_sign || cell == Sign::WALL) { blocked_op = true; break; }
                        if (cell == opponent_sign) count_op++;
                    }
                    
                    if (!blocked_op && count_op > 0) {
                        if (count_op == win_len - 1) { must_immediate_def = true; }                        
                        def_dir_weight = check_line(state, start_x, start_y, dx, dy, win_len, false);
                    }

                    move.atk_weight += atk_dir_weight;
                    move.def_weight += def_dir_weight;
                }
                
            }
            move.weight = (move.atk_weight * ATK_COEFF) + (move.def_weight * DEF_COEFF);
            if (can_immediate_win)       immediate_win.push_back(move);
            if (can_immediate_open_four) immediate_open_four.push_back(move);
            if (must_immediate_def)      immediate_def.push_back(move);
            
            moves.push_back(move);
        }
    }
    
    // для сортировки векторов
    auto weight_comparator = [](const RatedMovePoint& a, const RatedMovePoint& b) {
        if (std::abs(a.weight - b.weight) > 0.0001) {
            return a.weight > b.weight;
        }
        return a.distance < b.distance;
    };
    auto atk_weight_comparator = [](const RatedMovePoint& a, const RatedMovePoint& b) {
        if (std::abs(a.atk_weight - b.atk_weight) > 0.0001) {
            return a.atk_weight > b.atk_weight;
        }
        return a.distance < b.distance;
    };
    auto def_weight_comparator = [](const RatedMovePoint& a, const RatedMovePoint& b) {
        if (std::abs(a.def_weight - b.def_weight) > 0.0001) {
            return a.def_weight > b.def_weight;
        }
        return a.distance < b.distance;
    };
         
    // ВЫБОР ХОДА ПО ПРАВИЛАМ
    // 2) Проверка на победу. Ищется клетка, которая завершит собственную линию до длины L. 
    //    То есть нужно найти линию L-1, которую можно завершить. Если такая клетка есть, ход делается туда. 
    //    Если таких клеток несколько выбираем с большим весом защиты.
    if (!immediate_win.empty()) {
        std::sort(immediate_win.begin(), immediate_win.end(), atk_weight_comparator);
        return immediate_win[0].point;
    }
    // 3) Предотвращение проигрыша. Ищется клетка, которая позволит сопернику следующим ходом завершить линию до длины L. 
    //    То есть нужно найти линию L-1, которую можно завершить. Если такая клетка есть, ход делается туда, чтобы её занять. 
    //    Если таких клеток несколько выбираем с большим весом атаки.
    if (!immediate_def.empty()) {
        std::sort(immediate_def.begin(), immediate_def.end(), def_weight_comparator);
        return immediate_def[0].point;
    }
    // 4) Проверка на победную четверку. Проверить можно ли сделать 4 в ряд с пустыми на концах. 
    //    Комбинация ведёт к выигрышу на следующем ходу. Если есть, то делаем. 
    //    Если таких клеток несколько выбираем с большим весом защиты.
    if (!immediate_open_four.empty()) {
        std::sort(immediate_open_four.begin(), immediate_open_four.end(), atk_weight_comparator);
        return immediate_open_four[0].point;
    }
    // 5) Из всех свободных клеток выбирается та, у которой итоговый вес оказался максимальным. 
    //    Если таких несколько, то выбираем клетку ближе к центру поля. Но если есть несколько клеток 
    //    с одинаковым весом в одной линии и их атакующий вес больше защитного, то ставим X не подряд, а с другого конца линии.
    if (!moves.empty()) {
        std::sort(moves.begin(), moves.end(), weight_comparator);
        //return moves[0].point;
        double first_weight = moves[0].weight;
        std::vector<RatedMovePoint> atk_moves; // Все атакующие ходы, делящие максимальный балл
        for (const auto& m : moves) {            
            if (std::abs(m.weight - first_weight) < 0.0001) {
                if (m.atk_weight > m.def_weight) atk_moves.push_back(m);
            } else {
                break; // Массив отсортирован по убыванию, дальше веса меньше
            }
        }
        if (atk_moves.size() > 1) {
           for (int dis = win_len-1; dis >= 0; dis--) { //цикл по дистанции между клетками от макс (win_len-1) до мин (0)
             for (size_t i = 0; i < atk_moves.size(); i++) {
                for (size_t j = i + 1; j < atk_moves.size(); j++) {
                    const auto& m1 = atk_moves[i];
                    const auto& m2 = atk_moves[j];
                    int dx = m2.point.x - m1.point.x;
                    int dy = m2.point.y - m1.point.y;
                    bool same_line = (dx == 0 || dy == 0 || std::abs(dx) == std::abs(dy));// m1 и m2 на одной линии
                    int distance = std::max(std::abs(dx), std::abs(dy));// расстояние между m1 и m2
                    if (same_line && distance == dis) {
                        int step_x = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
                        int step_y = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);
                        int near_m1 = 0; // количество наших крестиков рядом с m1
                        int near_m2 = 0; // количество наших крестиков рядом с m2
                        // Проверяем клетки, которая находится рядом с точкой m1
                        int check_x1 = m1.point.x + step_x;
                        int check_y1 = m1.point.y + step_y;
                        int check_x2 = m1.point.x - step_x;
                        int check_y2 = m1.point.y - step_y;
                        if (check_x1 >= 0 && check_x1 < cols && check_y1 >= 0 && check_y1 < rows) {
                            if (state.get_value(check_x1, check_y1)== m_sign) near_m1++;                            
                        }
                        if (check_x2 >= 0 && check_x2 < cols && check_y2 >= 0 && check_y2 < rows) {
                            if (state.get_value(check_x2, check_y2)== m_sign) near_m1++;
                        }
                        // Проверяем клетки, которая находится рядом с точкой m2
                        check_x1 = m2.point.x + step_x;
                        check_y1 = m2.point.y + step_y;
                        check_x2 = m2.point.x - step_x;
                        check_y2 = m2.point.y - step_y;
                        if (check_x1 >= 0 && check_x1 < cols && check_y1 >= 0 && check_y1 < rows) {
                            if (state.get_value(check_x1, check_y1)== m_sign) near_m2++;                     
                        }
                        if (check_x2 >= 0 && check_x2 < cols && check_y2 >= 0 && check_y2 < rows) {
                            if (state.get_value(check_x2, check_y2)== m_sign) near_m2++;
                        }
                        if (near_m1 > near_m2) {
                            // Если рядом с m1 наших крестиков больше чем у m2, значит m2 — это другой свободный конец линии!
                            return m2.point;
                        }
                        else{
                            // Если рядом с m2 наших крестиков больше чем у m1, значит m1 — это другой свободный конец линии!
                            return m1.point;
                        }
                    }
                }
             }
           }
        }
        return moves[0].point;
    }    
    return Point{0, 0}; // нет свободных клеток, возвращаем 0,0 
}

void MyPlayer2::set_sign(Sign sign) { m_sign = sign; }
const char *MyPlayer2::get_name() const { return m_name; }

Point MyPlayer2::make_move(const State &state) {  
  Point result;
  for (int n_attempt = 0; n_attempt < 50; ++n_attempt) {
    result.x = std::rand() % state.get_opts().cols;
    result.y = std::rand() % state.get_opts().rows;
    if (state.get_value(result.x, result.y) != Sign::NONE) {
      --n_attempt;
      continue;
    }
    bool has_neighbors = false;
    for (int dx = -1; dx <= 1; ++dx) {
      for (int dy = -1; dy <= 1; ++dy) {
        if (dx == 0 && dy == 0)
          continue;
        const Sign val = state.get_value(result.x + dx, result.y + dy);
        if (val == Sign::X || val == Sign::O) {
          has_neighbors = true;
          break;
        }
      }
      if (has_neighbors)
        break;
    }
    if (has_neighbors)
      break;
  }
  return result;
}

}; // namespace ttt::my_player
