#include <iostream>
#include <chrono>
#include <vector>
#include <unordered_map>
#include "davidcache.h"

class LFUCacheMark {
    private:
        struct KeyVal {
            int key;
            int val;
            KeyVal* prevkv;
            KeyVal* nextkv;
        };

        struct Sublist {
            uint64_t uses;
            KeyVal* most;
            KeyVal* least;
            Sublist* prevsl;
            Sublist* nextsl;
        };

        std::unordered_map<int,std::pair<Sublist*,KeyVal*>> cachemap;

        Sublist* head;

        int maxcap;

        // Returns the sublist that fkv ends up in
        Sublist* increment(Sublist* fsl, KeyVal* fkv) {
            // Try and increment uses if fsl just contains fkv
            if (fsl->most == fkv and fsl->least == fkv) {
                fsl->uses++;
                // If next has same uses we need to move fkv and delete fsl
                if (fsl->nextsl != nullptr and fsl->nextsl->uses == fsl->uses) {
                    // Move fkv
                    Sublist* old = fsl;
                    fsl = fsl->nextsl;
                    fkv->nextkv = fsl->most;
                    fsl->most->prevkv = fkv;
                    fsl->most = fkv;
                    // Unlink old
                    if (old->prevsl != nullptr)
                        old->prevsl->nextsl = old->nextsl;
                    else
                        head = old->nextsl;
                    if (old->nextsl != nullptr)
                        old->nextsl->prevsl = old->prevsl;
                    delete old;
                }
                return fsl;
            }
            // Otherwise unlink and insert into correct sublist
            else {
                // Unlink from found sublist
                if (fkv->prevkv != nullptr)
                    fkv->prevkv->nextkv = fkv->nextkv;
                else
                    fsl->most = fkv->nextkv;
                if (fkv->nextkv != nullptr)
                    fkv->nextkv->prevkv = fkv->prevkv;
                else
                    fsl->least = fkv->prevkv;
                // Update fkv next and prev
                fkv->prevkv = nullptr;
                fkv->nextkv = nullptr;
                // Insert into uses+1 sublist
                if (fsl->nextsl == nullptr)
                    fsl->nextsl = new Sublist{fsl->uses + 1, fkv, fkv, fsl, nullptr};
                else if (fsl->nextsl->uses == fsl->uses + 1) {
                    fsl->nextsl->most->prevkv = fkv;
                    fkv->nextkv = fsl->nextsl->most;
                    fsl->nextsl->most = fkv;
                }
                else {
                    fsl->nextsl = new Sublist{fsl->uses + 1, fkv, fkv, fsl, fsl->nextsl};
                    if (fsl->nextsl->nextsl != nullptr)
                        fsl->nextsl->nextsl->prevsl = fsl->nextsl;
                }
                return fsl->nextsl;
            }
        }

    public:
        LFUCacheMark(int capcacity) {
            cachemap.reserve(capcacity);
            maxcap = capcacity;
            head = nullptr;
        }

        ~LFUCacheMark(void) {
            while (head != nullptr) {
                while (head->most != nullptr) {
                    KeyVal* okv = head->most;
                    head->most = head->most->nextkv;
                    delete okv;
                }
                Sublist* old = head;
                head = head->nextsl;
                delete old;
            }
        }

        int get(int key) {
            auto mapres = cachemap.find(key);
            if (mapres != cachemap.end()) {
                // Increment uses if found
                Sublist* fsl = mapres->second.first;
                KeyVal* fkv = mapres->second.second;
                fsl = increment(fsl, fkv);
                cachemap[key] = std::make_pair(fsl, fkv);
                return fkv->val;
            }
            return -1;
        }

        void put(int key, int val) {
            if (maxcap <= 0)
                return;
            auto mapres = cachemap.find(key);
            // If key is already in the cache then just increment its uses
            if (mapres != cachemap.end()) {
                Sublist* fsl = mapres->second.first;
                KeyVal* fkv = mapres->second.second;
                fsl = increment(fsl, fkv);
                fkv->val = val;
                cachemap[key] = std::make_pair(fsl, fkv);
            }
            else {
                KeyVal* nkv;
                // Create a new KeyVal if there is space
                if (cachemap.size() < maxcap)
                    nkv = new KeyVal();
                // Otherwise evict (Reuse the evictee)
                else {
                    cachemap.erase(head->least->key);
                    nkv = head->least;
                    // Unlink least recently used from head;
                    head->least = nkv->prevkv;
                    if (nkv->prevkv != nullptr)
                        nkv->prevkv->nextkv = nullptr;
                    else
                        head->most = nullptr;
                    // Delete head if it's empty
                    if (head->most == nullptr and head->least == nullptr) {
                        Sublist* old = head;
                        head = head->nextsl;
                        delete old;
                    }
                }
                // (Re)set members of nkv
                nkv->key = key;
                nkv->val = val;
                nkv->prevkv = nullptr;
                nkv->nextkv = nullptr;
                // Insert nkv into correct head
                if (head == nullptr)
                    head = new Sublist{1, nkv, nkv, nullptr, nullptr};
                else if (head->uses > 1) {
                    head->prevsl = new Sublist{1, nkv, nkv, nullptr, head};
                    head = head->prevsl;
                }
                else {
                    head->most->prevkv = nkv;
                    nkv->nextkv = head->most;
                    head->most = nkv;
                }
                cachemap[key] = std::make_pair(head, head->most);
            }
        }

        void print(void) {
            Sublist* csl = head;
            while (csl != nullptr) {
                std::cout << "[ " << csl->uses << " ( ";
                KeyVal* ckv = csl->most;
                while (ckv != nullptr) {
                    std::cout << ckv->key << " ";
                    ckv = ckv->nextkv;
                }
                std::cout << ") ] ";
                csl = csl->nextsl;
            }
            std::cout << std::endl;
        }
};

int main(void) {

    // Preload the ops
    uint64_t numops;
    std::cin >> numops;

    std::vector<std::pair<char,std::pair<int,int>>> ops (numops);

    for (int i = 0; i < ops.size(); i++) {
        char op;
        int key, val;
        std::cin >> op;
        if (op == 'g')
            std::cin >> key;
        else
            std::cin >> key >> val;
        ops[i] = std::make_pair(op, std::make_pair(key, 0));
    }

    // Actually run
    auto start = std::chrono::high_resolution_clock::now();

    // LFUCacheMark cache (10);

    // for (int i = 0; i < ops.size(); i++) {
    //     if (ops[i].first == 'g')
    //         cache.get(ops[i].second.first);
    //     else
    //         cache.put(ops[i].second.first, ops[i].second.second);
    // }

    LFUCache *cache = lFUCacheCreate(10);

    for (int i = 0; i < ops.size(); i++) {
        if (ops[i].first == 'g')
            lFUCacheGet(cache, ops[i].second.first);
        else
            lFUCachePut(cache, ops[i].second.first, ops[i].second.second);
    }

    auto stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> runtime = stop - start;

    std::cout << "Runtime " << runtime.count() << " seconds" << std::endl;

    return 0;
}