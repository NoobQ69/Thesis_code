#pragma GCC push_options
#pragma GCC optimize ("O0")
#include "Arduino.h"
#include "linked_list.h"

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