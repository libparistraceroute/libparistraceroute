#ifndef H_PATR_FIELDLIST
#define H_PATR_FIELDLIST

#include "field.h"

typedef struct fieldlist_struct{
	/** name of the field (port dest,...)*/ char* name;
	/** value of the field*/field_s* value;
	/** next option*/ struct fieldlist_struct *next;
} fieldlist_s;

/**
 * create a new fieldlist (alone, to begin a list)
 * @param field_name the name of the fieldlist to set
 * @param new_value the value to set : a pointer to an ALREADY ALLOCATED fields
 * @return new pointer to fieldlist
 */

fieldlist_s* create_fieldlist(char* field_name, field_s* new_value);

/**
 * add the field list in the list of field list
 * @param list_of_list the list in which add
 * @param name the name of this list (ot found this list between others
 * @param list the new fields list
 * @return new pointer to fieldlist
 */
fieldlist_s* add_fieldlist(fieldlist_s* list_of_lists, char* name, field_s* list);

/**
 * set a field to a certain list of fields, freeing the precedent list!
 * @param fields the fields list
 * @param field_name the name of the field to set
 * @param new_value the value to set : a pointer to the new field list
 * @return 1 if succes, 0 otherwise
 */
int set_fieldlist_value(fieldlist_s* fields, char* field_name, field_s* new_value);

/**
 * update a field to a certain list of fields, i e does not free the precent content
 * @param fields the fields list
 * @param field_name the name of the field to set
 * @param new_value the value to set : a pointer to the new field list
 * @return 1 if succes, 0 otherwise
 */
int update_fieldlist_value(fieldlist_s* fields, char* field_name, field_s* new_value);

/**
 * get a certain list from the field list list
 * @param fields the fields list
 * @param field_name the name of the field to qet
 * @return the value (need to cast after)
 */
field_s* get_fieldlist_value(fieldlist_s* fields, char* field_name);

/**
 * get a value from the field list list
 * @param fields the fields list
 * @param field_name the name of the field to qet
 * @return the value (need to cast after)
 */
void* get_field_value_from_fieldlist(fieldlist_s* fields, char* field_name);

/**
 * debug function to see the content of a field list list
 * @param fields the fields list
 * @return void
 */

void view_fieldlist(fieldlist_s* fields);

/**
check presence of a fieldlist in a fieldlist list CAUTION, different from checking the presence of a list of fields in the fieldlist list CAUTION, different from checking rpesence of a field
 * 
 * @param field_name the name of the field to check
 * @param fields the fields list list
 * @return the value : -1 if not found, 1 otherwise
 */

int check_presence_fieldlist(char* fieldlist_name,fieldlist_s* fields);

/**
 * check presence of a list of fieldlist CAUTION different from checking presence of a list of fields
 * @param fields_names the name of the field to check
 * @param size the size of the array list
 * @param fields the fields list list
 * @return the value : -1 if not at least one is not found, 1 otherwise
 */
int check_presence_list_fieldlist(char** fieldlists_names, int size, fieldlist_s* fields);

/**
 * check presence of a field (from its name) in a fieldlist list CAUTION, different from checking the presence of a fieldlist in the fieldlist list
 * @param field_names the name of the field to check
 * @param fields the fieldslist list
 * @return the value : -1 if not at least one is not found, 1 otherwise
 */

int check_presence_field(char* field_name,fieldlist_s* fieldlist);

/**
 * check presence of a list of fields (from their name) in a fieldlist list CAUTION, different from checking the presence of a fieldlist list in the fieldlist list
 * @param fields_names the name of the field to check
 * @param size the size of the array list
 * @param fields the fields list list
 * @return the value : -1 if not at least one is not found, 1 otherwise
 */
int check_presence_list_of_fields(char** field_list_name, int size, fieldlist_s* fields);
/**
 * remove a fieldlist from the list of fieldlists
 * @param fieldlist the list of fieldlist
 * @param field_name the fieldlist to remove
 * @param with_content indicate if content must be freed 0 false, else true
 * @return int 1 if succes, -1 otherwise
 */
int remove_fieldlist(fieldlist_s** fields, char* field_name, int with_content);

/**
 * remove a field from the list of fieldlists, if fieldlist_name is null, all instances in all fieldlists are removed
 * @param fieldlist the list of fieldlist
 * @param field_name the fieldlist to remove
 * @param fieldlist_name optional, the fieldlist in which try to remove the field
 * @param with_content indicates if content must be freed 0 false else true
 * @return int 1 if succes, -1 otherwise
 */
int remove_field_from_fieldlist(fieldlist_s** fields, char* field_name,char* fieldlist_name, int with_content);

/**
 * free all fieldlists(and their pointed values)
 * @param fields the fields list
 * @return void
 */
void free_all_fieldlists(fieldlist_s* fields);

/**
 * free all fieldlists(and their pointed values)
 * @param fields the fields list
 * @return void
 */
void free_all_fieldlists_with_function(fieldlist_s* fields, void (*free_func)(void* elem));

/**
 * free a fieldlist (when removed for example) (and their pointed values)
 * @param fields the fieldslist
 * @param with_content indicates if the field_s must be freed, 0=false, else=true
 * @return void
 */
void free_fieldlist(fieldlist_s* fields,int with_content);

/**
 * get the number of fields in  a list of fields
 * @param fields the field list
 * @return the number of fields
 */
int get_number_of_fieldlists(fieldlist_s* fields);

#endif
