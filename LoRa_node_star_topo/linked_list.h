#ifndef _LINKED_LIST_H__
#define _LINKED_LIST_H__

/* the trivial linked list that is specific for queue data structure in LoRa_star_topo_service.h file */

#include "string.h"

struct LL_Node 
{
    char *value;
    struct LL_Node *pointer;
};
typedef struct LL_Node NODE;

typedef class LinkedList // Class
{
private:
    NODE *List;
    int N;
public:
  LinkedList();
  ~LinkedList();
  void add_tail(char *val);
  void add_head(char *val);
  // void pre_add(double val, double present);
  // void sub_add(double val, double present);
  void delete_head();
  void delete_tail();
  int get_len();
  void get_payload_head(char *data);
  int is_empty();
  // void delete_target(double val);
  // void searching(double val);
  void print();
  void delete_all();
} LIST;


#endif

