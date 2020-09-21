// move your cursor up/down, left/right inside the terminal. use the ideology of immutability
// to rerender screens
// need libncurses. compile with -lncurses
#include <ncurses.h>

#include "store.hh"

namespace {

struct player_state {
    int x, y, maxx, maxy;
};

enum class player_actions {
    UP, DOWN, LEFT, RIHGT, RESIZE
};

auto win_reducer(const player_state &state, player_actions action) {
    auto newstate = state;
    switch (action) {
    case player_actions::UP: {
        if (state.y > 0) --newstate.y;
        break; }
    case player_actions::DOWN: {
        if (state.y < state.maxy) ++newstate.y;
        break; }
    case player_actions::LEFT: {
        if (state.x > 0) --newstate.x;
        break; }
    case player_actions::RIHGT: {
        if (state.x < state.maxx) ++newstate.x;
        break; }
    case player_actions::RESIZE: default: {
        newstate.maxx = getmaxx(stdscr) - 3;
        newstate.maxy = getmaxy(stdscr) - 2;
        // place cursor back into the window if overflow
        if (state.y > newstate.maxy) newstate.y = newstate.maxy;
        if (state.x > newstate.maxx) newstate.x = newstate.maxx;
        break; }
    }
    return newstate;
}

auto redraw_scr(const player_state &state) {
    erase(); // instead of clear(): prevent flickering
    attron(COLOR_PAIR(1));
    // +3 to restore the original max screen width (line 34)
    for (int i = 0; i < state.maxx + 3; ++i) printw(" ");
    mvprintw(0, 0, "Position: (%d, %d)", state.x, state.y);
    mvprintw(0, state.maxx - 14, "Press Q to exit");
    attroff(COLOR_PAIR(1));
    use_default_colors();
    mvprintw(state.y + 1, state.x, "[*]");
    refresh();
}

}

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, true); // allow arrow keys
    curs_set(false);

    start_color();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);

    auto store = myrdx::store<player_state, player_actions>(
        win_reducer,
        player_state { 0, 0, 0, 0 }
    );
    store.subscribe([&store] {
        redraw_scr(store.get_state());
    });

    store.dispatch(player_actions::RESIZE);

    while (true) {
        auto kb = getch();
        switch (kb) {
        case 'w': case KEY_UP:
            store.dispatch(player_actions::UP);
            break;
        case 's': case KEY_DOWN:
            store.dispatch(player_actions::DOWN);
            break;
        case 'a': case KEY_LEFT:
            store.dispatch(player_actions::LEFT);
            break;
        case 'd': case KEY_RIGHT:
            store.dispatch(player_actions::RIHGT);
            break;
        case KEY_RESIZE:
            store.dispatch(player_actions::RESIZE);
            break;
        case 'q':
            goto end;
        }
    }
end:
    endwin();
}
