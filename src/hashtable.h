#pragma once
#include <cstdlib>
#include <cassert>
#include <cstdint>

struct HNode {
    HNode* next = nullptr;
    uint64_t hcode = 0;
};

struct HTab {
    HNode** tab = nullptr;
    size_t mask = 0;
    size_t size = 0;
};

inline void h_init(HTab* ht, size_t n) {
    assert((n & (n - 1)) == 0); // power of 2
    ht->tab = (HNode**)calloc(n, sizeof(HNode*));
    ht->mask = n - 1;
    ht->size = 0;
}

inline void h_insert(HTab* ht, HNode* node) {
    size_t pos = node->hcode & ht->mask;
    node->next = ht->tab[pos];
    ht->tab[pos] = node;
    ht->size++;
}

inline HNode** h_lookup(HTab* ht, HNode* key, bool (*eq)(HNode*, HNode*)) {
    size_t pos = key->hcode & ht->mask;
    HNode** from = &ht->tab[pos];
    while (*from) {
        if (eq(*from, key)) return from;
        from = &(*from)->next;
    }
    return nullptr;
}

inline HNode* h_detach(HTab* ht, HNode** from) {
    HNode* node = *from;
    *from = node->next;
    ht->size--;
    return node;
}
