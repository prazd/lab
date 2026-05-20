#ifndef UI_H
#define UI_H

template <size_t x, size_t y, size_t w, size_t h>
struct UserInput {
    std::stringstream ss;
    std::string s;
    std::string selected_sub;
    std::string offset_sub;
    Rectangle rec;
    size_t cursor_pos = 0;
    float wsize = 15.f;

    int selected_begin = -1;
    int selected_end = -1;
    UserInput(): rec{x, y, w, h} {}
    
    void flush_select() {
        selected_begin = -1; selected_end = -1; selected_sub.clear(); offset_sub.clear();
    }
    void push_handler() {
        if (!CheckCollisionPointRec(GetMousePosition(), rec)) return;
        if (IsKeyPressed(KEY_LEFT))  {flush_select(); cursor_pos += cursor_pos ? -1 : 0;}
        if (IsKeyPressed(KEY_RIGHT)) {flush_select(); cursor_pos += cursor_pos < s.size() ? 1 : 0;}
        if (IsKeyPressed(KEY_V) && IsKeyDown(KEY_LEFT_SUPER)) {s = GetClipboardText(); cursor_pos = s.size();}
        
        if (IsKeyPressed(KEY_A) && IsKeyDown(KEY_LEFT_SUPER)) {if (!s.empty()) {selected_begin = 0; selected_end = s.size() + 1; selected_sub = s.substr(selected_begin, selected_end); offset_sub.clear();}}
        
        if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyDown(KEY_LEFT_SUPER) && IsKeyPressed(KEY_LEFT)) {if (!s.empty())  {selected_begin = 0; selected_end = cursor_pos + 1; cursor_pos = 0; selected_sub = s.substr(selected_begin, selected_end);}}
        else if (IsKeyDown(KEY_LEFT_SUPER) && IsKeyPressed(KEY_LEFT) && !IsKeyPressed(KEY_LEFT_SHIFT)) {if (!s.empty()) {flush_select(); cursor_pos = 0;}}
        
        if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyDown(KEY_LEFT_SUPER) && IsKeyPressed(KEY_RIGHT)) {if (!s.empty()) {selected_begin = cursor_pos ? cursor_pos-1 : 0; selected_end = s.size() + 1; cursor_pos = s.size(); selected_sub = s.substr(selected_begin, selected_end);  offset_sub = s.substr(0, selected_begin);}}
        else if (IsKeyDown(KEY_LEFT_SUPER) && IsKeyPressed(KEY_RIGHT) && !IsKeyPressed(KEY_LEFT_SHIFT)) {if (!s.empty()) {flush_select(); cursor_pos = s.size();}}

        if (IsKeyPressed(KEY_C) && IsKeyDown(KEY_LEFT_SUPER)) {SetClipboardText(selected_sub.c_str());}
    
        auto c = GetCharPressed();
        if (!c) return;
        if (selected_begin != -1 && selected_end != -1){
            cursor_pos = 0;
            flush_select();
            s.clear();
        }
        s.insert(cursor_pos, 1, (char)c);
        cursor_pos+=1;
    }
    void pop_handler() {
        if (!CheckCollisionPointRec(GetMousePosition(), rec)) return;
        if (IsKeyPressed(KEY_BACKSPACE) && !s.empty()){
            if (selected_begin != -1 && selected_end != -1){
                s.erase(selected_begin, selected_end-selected_begin);
                if (selected_begin != 0)
                    cursor_pos = s.size();
                else 
                    cursor_pos = 0;    
                flush_select();
            } else {
                if (cursor_pos){
                    s.erase(cursor_pos - (cursor_pos ? 1 : 0), 1);
                    --cursor_pos;
                    flush_select();
                }    
            }
        }
    }
    void handle() {
        push_handler();
        pop_handler();
        DrawRectangleLinesEx({rec.x, rec.y, rec.width, rec.height}, 1.f, BLACK);
        if (selected_begin != -1) {
            auto twidth = MeasureText(selected_sub.c_str(), wsize);
            auto offset_width = MeasureText(offset_sub.c_str(), wsize);
            DrawRectangle(rec.x + offset_width, rec.y, twidth, wsize, BLUE);
#if 0            
            std::cout << "Selected_Begin:" << selected_begin << " SelectedEnd:" << selected_end << 
            " Twidth:" << twidth << " Diff:" << selected_end-selected_begin << std::endl;
#endif            
        }
            
        for (size_t i = 0, e = s.size(); i != e; ++i){
            if (i == cursor_pos && CheckCollisionPointRec(GetMousePosition(), rec))
                ss << "|";

            ss << s[i];

            if (cursor_pos == i + 1 && cursor_pos == s.size() && CheckCollisionPointRec(GetMousePosition(), rec))
                ss << "|";
        }
        if (s.empty() && CheckCollisionPointRec(GetMousePosition(), rec)){
            ss << " |";
        }
        auto r_str = ss.str();
        DrawText(r_str.data(), rec.x, rec.y, wsize, BLACK);
        ss.str("");
    }
};
#endif