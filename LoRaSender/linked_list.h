#ifndef _LINKED_LIST_H__
#define _LINKED_LIST_H__

#include "string.h"

struct Node 
{
    char *value;
    struct Node *pointer;
};
typedef struct Node NODE;

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

LinkedList::LinkedList() // Constructor
{
  List = NULL;
  N = 0;
}

LinkedList::~LinkedList() // Destructor
{
  for(int i = 0;i < N;i++)
  {
    NODE *temp = List;
    List = List->pointer;
    delete temp->value;
    delete temp;
  }
  Serial.println("List node deleted");
}

int LinkedList::get_len()
{
  return N;
}

NODE* CreateNode(char *val)
{
  NODE *newnode = new NODE;
  int len = strlen(val);
  newnode->value = new char[len];
  strcpy(newnode->value, val);
  newnode->pointer = NULL;
  return newnode;
}

void LinkedList::add_head(char *val)
{
    if(List == NULL)
    {
      List = CreateNode(val);
    }
    else
    {
      NODE *newnode = CreateNode(val);
      newnode->pointer = List;
      List = newnode;
    }
    N++;
}

void LinkedList::add_tail(char *val)
{
    if(List == NULL)
    {
        List = CreateNode(val);
    }
    else
    {
        NODE *newnode = CreateNode(val);
        NODE *i = List;
        while(i->pointer != NULL)
        {
            i = i->pointer;
        }
        i->pointer = newnode;
    }
    N++;
}

int LinkedList::is_empty()
{
  if (List == NULL)
  {
    return 1;
  }
  return 0;
}

// void LinkedList::pre_add(char *val, double present)
// {
//     if(List == NULL)
//     {
//         cout << "\nList is empty !";
//     }
//     else if(List->value == present)
//     {
//         NODE *temp = CreateNode(val);
//         temp->pointer = List;
//         List = temp;
//     }
//     else
//     {
//         NODE *temp = List;
//         int flag = 0;
//         while(temp->pointer != NULL)
//         {
//             if(temp->pointer->value == present)
//             {
//                 flag = 1;
//                 NODE *pre = CreateNode(val);
//                 pre->pointer = temp->pointer;
//                 temp->pointer = pre;
//                 break;
//             }
//             temp = temp->pointer;
//         }
//         if(!flag)
//         {
//             cout << "\nThere is no suitable value to add the list !";
//         }
//     }
// }

// void LinkedList::sub_add(double val,double present)
// {
//     if(List == NULL)
//     {
//         cout << "\nList is empty !";
//     }
//     else
//     {
//         NODE *temp = List;
//         int flag = 0;
//         while(temp != NULL)
//         {
//             if(temp->value == present)
//             {
//                 flag = 1;
//                 NODE *sub = CreateNode(val);
//                 if(temp->pointer == NULL)
//                 {
//                     temp->pointer = sub;
//                 }
//                 else
//                 {
//                     sub->pointer = temp->pointer;
//                     temp->pointer = sub;
//                 }
//             }
//             temp = temp->pointer;
//         }
//         if(!flag)
//         {
//             cout << "\nThere is no suitable value to add the list !";
//         }
//     }
// }

void LinkedList::get_payload_head(char *data)
{
  if (List != NULL)
  {
    strcpy(data, List->value);
  }
}

void LinkedList::delete_head()
{
    if(List == NULL)
    {
        Serial.println("List is empty !");
    }
    else
    {
        NODE * temp = List;
        if (List->pointer == NULL)
        {
          List = NULL;
        }
        else
        {
          List = List->pointer;
        }
        // delete temp->value;
        // temp->value = NULL;
        delete temp;
        N--;
    }
}

// void LinkedList::delete_tail()
// {
//     if(List == NULL)
//     {

//         cout << "\nList is empty !";
//     }
//     else
//     {
//         NODE *i = List;
//         NODE *follow = NULL;
//         while(i->pointer != NULL)
//         {
//             follow = i;
//             i = i->pointer;
//         }
//         follow->pointer = NULL;
//         delete i;
//         N--;
//     }
// }

// void LinkedList::delete_target(double val)
// {
//     if(List == NULL)
//     {
//         cout << "\nList is empty !";
//     }
//     else if(List->value == val)
//     {
//         NODE *temp = List;
//         List = List->pointer;
//         delete temp;
//     }
//     else
//     {
//         NODE *temp = List;
//         int flag(0);
//         while(temp->pointer != NULL)
//         {
//             if(temp->pointer->value == val)
//             {
//                 flag = 1;
//                 NODE *save = temp->pointer;
//                 if(temp->pointer->pointer == NULL)
//                 {
//                     temp->pointer = NULL;
//                 }
//                 else
//                 {
//                     temp->pointer = temp->pointer->pointer;
//                 }
//                 delete save;
//                 N--;
//                 break;
//             }
//             temp = temp->pointer;
//         }
//         if(!flag)
//         {
//             cout << "\nThere is no value suitable for your value to delete !";
//         }
//     }
// }

// void LinkedList::searching(double val)
// {
//     NODE *i = List;
//     while(i != NULL)
//     {
//         if(i->value == val)
//         {
//             cout << "\nValue "<< val << " is stored in the list";
//             return;
//         }
//         i = i->pointer;
//     }
//     cout << "\nValue "<< val << " is not stored in the list";
// }

void LinkedList::print()
{
    if(List == NULL)
    {
        Serial.println("List is empty !");
        return;
    }
    for(NODE *i = List;i != NULL;i = i->pointer)
    {
      Serial.printf("%s\n",i->value);
    }
}

void LinkedList::delete_all()
{
    while(List != NULL)
    {
      NODE *temp = List;
      List = List->pointer;
      delete temp;
    }
}

#endif

