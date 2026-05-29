#include "my_player.hpp"
#include <cstdlib>
#include <iostream>
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
            return -1; 
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
    
    int weight = (count > 1) ? (count - 1) : 0;
    if (count > 1 && open_before && open_after) weight++; //+1 если открыта
    return weight;
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
                
                int max_atk_dir_weight = 0;
                int max_def_dir_weight = 0;
                
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
                    
                    if (!blocked_my) {
                        if (count_my == win_len - 1) { can_immediate_win = true; }
                        if (win_len == 5 && count_my == 3) {
                            int res = check_line(state, start_x, start_y, dx, dy, win_len, true);
                            if (open_before && open_after) { can_immediate_open_four = true; }
                        }
                        
                        int res = check_line(state, start_x, start_y, dx, dy, win_len, true);
                        if (res > max_atk_dir_weight) {
                            max_atk_dir_weight = res;
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
                    
                    if (!blocked_op) {
                        if (count_op == win_len - 1) { must_immediate_def = true; }
                        
                        int res = check_line(state, start_x, start_y, dx, dy, win_len, false);
                        if (res > max_def_dir_weight) {
                            max_def_dir_weight = res;
                        }
                    }
                }
                
                move.atk_weight += max_atk_dir_weight;
                move.def_weight += max_def_dir_weight;
            }
            
            move.weight = (move.atk_weight * ATK_COEFF) + (move.def_weight * DEF_COEFF);
            
            if (can_immediate_win)       immediate_win.push_back(move);
            if (can_immediate_open_four) immediate_open_four.push_back(move);
            if (must_immediate_def)      immediate_def.push_back(move);
            
            moves.push_back(move);
        }
    }
    
    // для сортировки векторов
    auto move_comparator = [](const RatedMovePoint& a, const RatedMovePoint& b) {
        if (std::abs(a.weight - b.weight) > 0.0001) {
            return a.weight > b.weight;
        }
        return a.distance < b.distance;
    };
    
    Point result {0, 0};  
    // ВЫБОР ХОДА ПО ПРАВИЛАМ
    // 1) Проверка на победу. Ищется клетка, которая завершит собственную линию до длины L. То есть нужно найти линию L-1, которую можно завершить. Если такая клетка есть, ход делается туда.
    if (!immediate_win.empty()) {
        std::sort(immediate_win.begin(), immediate_win.end(), move_comparator);
        result=immediate_win[0].point;
    }
    // 2) Предотвращение проигрыша. Ищется клетка, которая позволит сопернику следующим ходом завершить линию до длины L. То есть нужно найти линию L-1, которую можно завершить. Если такая клетка есть, ход делается туда, чтобы её занять.
    if (!immediate_def.empty()) {
        std::sort(immediate_def.begin(), immediate_def.end(), move_comparator);
        result=immediate_def[0].point;
    }
    // 3) Проверка на победную четверку. Проверить можно ли сделать 4 в ряд с пустыми на концах. Комбинация ведёт к выигрышу на следующем ходу. Если есть, то делаем.
    if (!immediate_open_four.empty()) {
        std::sort(immediate_open_four.begin(), immediate_open_four.end(), move_comparator);
        result=immediate_open_four[0].point;
    }
    // 8) Из всех свободных клеток выбирается та, у которой итоговый вес оказался максимальным. Если таких несколько, то выбираем клетку ближе к центру поля.
    if (!moves.empty()) {
        std::sort(moves.begin(), moves.end(), move_comparator);
        result=moves[0].point;
        //if (m_sign==Sign::X){
        //    std::cout<<"XXX x="<<result.x<<" y="<<result.y<<" a="<<moves[0].atk_weight<<" d="<<moves[0].def_weight<<std::endl;
        //}
        //else{
        //    std::cout<<"OOO x="<<result.x<<" y="<<result.y<<" a="<<moves[0].atk_weight<<" d="<<moves[0].def_weight<<std::endl;
        //}
    }    
    return result;   
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
  //result.x=2;
  //result.y=temp++;
  //if (temp==4) temp++;
  return result;
}

}; // namespace ttt::my_player
