#include "Frontend.h"

#include <cstring>
#include <iostream>

int Frontend::create_table(char relname[ATTR_SIZE], int no_attrs, char attributes[][ATTR_SIZE],
                           int type_attrs[]) {
  // Schema::createRel
  // return SUCCESS;
  return Schema::createRel(relname, no_attrs, attributes, type_attrs);
}

int Frontend::drop_table(char relname[ATTR_SIZE]) {
  // Schema::deleteRel
  // return SUCCESS;
  return Schema::deleteRel(relname);
}

int Frontend::open_table(char relname[ATTR_SIZE]) {
  // Schema::openRel
  // return SUCCESS;
  return Schema::openRel(relname);
}

int Frontend::close_table(char relname[ATTR_SIZE]) {
  // Schema::closeRel
  // return SUCCESS;
  return Schema::closeRel(relname);
}

int Frontend::alter_table_rename(char relname_from[ATTR_SIZE], char relname_to[ATTR_SIZE]) {
  // Schema::renameRel
  // return SUCCESS;
    return Schema::renameRel(relname_from, relname_to);
}

int Frontend::alter_table_rename_column(char relname[ATTR_SIZE], char attrname_from[ATTR_SIZE],
                                        char attrname_to[ATTR_SIZE]) {
  // Schema::renameAttr
  // return SUCCESS;
  return Schema::renameAttr(relname, attrname_from, attrname_to);
}

int Frontend::create_index(char relname[ATTR_SIZE], char attrname[ATTR_SIZE]) {
  // Schema::createIndex
  // return SUCCESS; 
  // Call createIndex() method of the Schema Layer with correct arguments
  // Return Success and Error values appropriately
  return Schema::createIndex(relname, attrname);
}

int Frontend::drop_index(char relname[ATTR_SIZE], char attrname[ATTR_SIZE]) {
  // Schema::dropIndex
  // return SUCCESS; 
  // Call dropIndex() method of the Schema Layer with correct arguments
  // Return Success and Error values appropriately
  return Schema::dropIndex(relname, attrname);
}

int Frontend::insert_into_table_values(char relname[ATTR_SIZE], int attr_count, char attr_values[][ATTR_SIZE]) {
  // Algebra::insert
  // return SUCCESS;
  return Algebra::insert(relname, attr_count, attr_values);
}

int Frontend::select_from_table(char relname_source[ATTR_SIZE], char relname_target[ATTR_SIZE]) {
  // Algebra::project
  // return SUCCESS;
  // Call appropriate project() method of the Algebra Layer
  // Return Success or Error values appropriately
  return Algebra::project(relname_source, relname_target);

}

int Frontend::select_attrlist_from_table(char relname_source[ATTR_SIZE], char relname_target[ATTR_SIZE],
                                         int attr_count, char attr_list[][ATTR_SIZE]) {
  // Algebra::project
  // return SUCCESS;
  // Call appropriate project() method of the Algebra Layer
  // Return Success or Error values appropriately
  return Algebra::project(relname_source, relname_target, attr_count, attr_list);
}

int Frontend::select_from_table_where(char relname_source[ATTR_SIZE], char relname_target[ATTR_SIZE],
                                      char attribute[ATTR_SIZE], int op, char value[ATTR_SIZE]) {
  // Algebra::select
  // return SUCCESS;
  return Algebra::select(relname_source, relname_target, attribute, op, value);

}

int Frontend::select_attrlist_from_table_where(char relname_source[ATTR_SIZE], char relname_target[ATTR_SIZE],
                                               int attr_count, char attr_list[][ATTR_SIZE],
                                               char attribute[ATTR_SIZE], int op, char value[ATTR_SIZE]) {
  // Algebra::select + Algebra::project??
  // return SUCCESS;

  // Call select() method of the Algebra Layer with correct arguments to
  // create a temporary target relation with name ".temp" (use constant TEMP)
  char tempStr[] = TEMP;
  int response = Algebra::select(relname_source, tempStr, attribute, op, value);

  // TEMP will contain all the attributes of the source relation as it is the
  // result of a select operation

  // Return Error values, if not successful
  if(response != SUCCESS) {
    return response;
  }

  // Open the TEMP relation using OpenRelTable::openRel()
  int tempRelId = OpenRelTable::openRel(tempStr);
  // if open fails, delete TEMP relation using Schema::deleteRel() and
  // return the error code
  if(tempRelId < 0) {
    Schema::deleteRel(tempStr);
    return tempRelId;
  }

  // On the TEMP relation, call project() method of the Algebra Layer with
  // correct arguments to create the actual target relation. The final
  // target relation contains only those attributes mentioned in attr_list
  response = Algebra::project(tempStr, relname_target, attr_count, attr_list);

  
  // close the TEMP relation using OpenRelTable::closeRel()
  OpenRelTable::closeRel(tempRelId);
  // delete the TEMP relation using Schema::deleteRel()
  Schema::deleteRel(tempStr);

  // return any error codes from project() or SUCCESS otherwise
  return response;
}

int Frontend::select_from_join_where(char relname_source_one[ATTR_SIZE], char relname_source_two[ATTR_SIZE],
                                     char relname_target[ATTR_SIZE],
                                     char join_attr_one[ATTR_SIZE], char join_attr_two[ATTR_SIZE]) {
  // Algebra::join
  // return SUCCESS;
  return Algebra::join(relname_source_one, relname_source_two, relname_target, join_attr_one, join_attr_two);
  
}

int Frontend::select_attrlist_from_join_where(char relname_source_one[ATTR_SIZE], char relname_source_two[ATTR_SIZE],
                                              char relname_target[ATTR_SIZE],
                                              char join_attr_one[ATTR_SIZE], char join_attr_two[ATTR_SIZE],
                                              int attr_count, char attr_list[][ATTR_SIZE]) {
  // Algebra::join + project
  // return SUCCESS;
  // Call join() method of the Algebra Layer with correct arguments to
  // create a temporary target relation with name TEMP.

  // TEMP results from the join of the two source relation (and hence it
  // contains all attributes of the source relations except the join attribute
  // of the second source relation)
  char tempStr[] = TEMP;
  int response = Algebra::join(relname_source_one, relname_source_two, tempStr, join_attr_one, join_attr_two);


  // Return Error values, if not successful
  if(response != SUCCESS) {
    return response;
  }

  // Open the TEMP relation using OpenRelTable::openRel()
  // if open fails, delete TEMP relation using Schema::deleteRel() and
  // return the error code
  int tempRelId = OpenRelTable::openRel(tempStr);
  if(tempRelId < 0) {
    Schema::deleteRel(tempStr);
    return tempRelId;
  }

  // Call project() method of the Algebra Layer with correct arguments to
  // create the actual target relation from the TEMP relation.
  // (The final target relation contains only those attributes mentioned in attr_list)
  response = Algebra::project(tempStr, relname_target, attr_count, attr_list);  

  if(response != SUCCESS) {
    OpenRelTable::closeRel(tempRelId);
    Schema::deleteRel(tempStr);
    return response;
  }

  // close the TEMP relation using OpenRelTable::closeRel()
  response = OpenRelTable::closeRel(tempRelId);

  if(response != SUCCESS) {
    printf("Error closing relation %s\n", tempStr);
    return response;
  }

  // delete the TEMP relation using Schema::deleteRel()
  response = Schema::deleteRel(tempStr);

  if(response != SUCCESS) {
    printf("Error deleting relation %s\n", tempStr);
    return response;
  }
  

  // Return Success or Error values appropriately
  return response;
}

int Frontend::custom_function(int argc, char argv[][ATTR_SIZE]) {
  // argc gives the size of the argv array
  // argv stores every token delimited by space and comma

  // implement whatever you desire
  return SUCCESS;
}