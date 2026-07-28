#pragma once

struct posi_t {
    int x;
    int y;
};

struct mouse_t {
    posi_t posi;
    bool pushing;
};