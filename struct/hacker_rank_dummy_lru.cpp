#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <set>
#include <cassert>
using namespace std;

struct Node{
   Node* next;
   Node* prev;
   int value;
   int key;
   Node(Node* p, Node* n, int k, int val):prev(p),next(n),key(k),value(val){};
   Node(int k, int val):prev(NULL),next(NULL),key(k),value(val){};
};

class Cache{
   
   protected: 
   map<int,Node*> mp; //map the key to the node in the linked list
   int cp;  //capacity
   Node* tail; // double linked list tail pointer
   Node* head; // double linked list head pointer
   virtual void set(int, int) = 0; //set function
   virtual int get(int) = 0; //get function

};

// todo: fix (unstable)
struct LRUCache: public Cache {
    LRUCache(int cap) {
        cp = cap;
    }
    int get(int key) override {
        if (mp.find(key) == mp.end()) {
            return -1;
        }
        auto el = mp.find(key);
        return el->second->value;
    }
    void set(int key, int value) override {
         if (mp.size() == 0) {
            Node* n = new Node(key, value);
            tail = n;
            head = n;
            mp[key] = n;
            return;
        }
        if (mp.find(key) != mp.end()) {
             auto key_val = mp.find(key);
             auto curr_head = mp.find(head->key);
             key_val->second->value = value;
             head = &*(key_val->second);
             curr_head->second->prev = head;
            return;
        }
         if (mp.size() < cp) {
                Node* n = new Node(nullptr, head, key, value);
                auto curr_head = mp.find(head->key);
                curr_head->second->prev = n;
                head = n;
                mp[key] = n;
                return;
        }
        if (mp.size() == cp) {
                Node* n = new Node(nullptr, head, key, value);
                auto prev_head = mp.find(head->key);
                prev_head->second->prev = n;
                head = n;
                mp[key] = n;
          
                auto old = mp.find(tail->key);
                old->second->prev->next = nullptr;
                auto new_tail = old->second->prev;
                Node* old_ptr = old->second;
                tail = new_tail;
                mp.erase(old->first);
                delete old_ptr;
                return;
        }
    }
   
};
int main() {
   int n, capacity,i;
   cin >> n >> capacity;
   LRUCache l(capacity);
   for(i=0;i<n;i++) {
      string command;
      cin >> command;
      if(command == "get") {
         int key;
         cin >> key;
         cout << l.get(key) << endl;
      } 
      else if(command == "set") {
         int key, value;
         cin >> key >> value;
         l.set(key,value);
      }
   }
   return 0;
}
