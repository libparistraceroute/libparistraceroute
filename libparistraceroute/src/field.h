#ifndef H_PATR_FIELD
#define H_PATR_FIELD

typedef struct field_struct{
	/** name of the field (port dest,...)*/ char* name;
	/** value of the field*/void* value;
	/** next option*/ struct field_struct *next;
} field_s;

/** caution ! for all the add/create option, if a field with the same name already exist, it is replaced and the old value is FREED so it can create segfault*/
/**
 * create a new field (alone, to begin a list)
 * @param fields the fields list in which add the field (may be NULL to init a list)
 * @param field_name the name of the field to set
 * @param new_value the value to set : a pointer to an ALREADY ALLOCATED space
 * @return new pointer to field_list
 */
field_s* create_field(char* field_name, void* new_value);

/**
 * add a new field in the beginning of a field list
 * @param fields the fields list in which add the field (may be NULL to init a list)
 * @param field_name the name of the field to set
 * @param new_value the value to set : a pointer to an ALREADY ALLOCATED space
 * @return new pointer to field_list
 */
field_s* add_field(field_s* next, char* field_name, void* new_value);

/**
 * add a new field in the beginning of a field list
 * @param fields the fields list in which add the field (may be NULL to init a list)
 * @param field_name the name of the field to set
 * @param new_value the int value to set, function will take care of malloc
 * @return new pointer to field_list
 */
field_s* add_int_field(field_s* next, char* field_name, int new_value);

/**
 * add a new field in the beginning of a field list
 * @param fields the fields list in which add the field (may be NULL to init a list)
 * @param field_name the name of the field to set
 * @param new_value the short value to set, function will take care of malloc
 * @return new pointer to field_list
 */
field_s* add_short_field(field_s* next, char* field_name, short new_value);

/**
 * add a new field in the beginning of a field list
 * @param fields the fields list in which add the field (may be NULL to init a list)
 * @param field_name the name of the field to set
 * @param new_value the char value to set, function will take care of malloc
 * @return new pointer to field_list
 */
field_s* add_char_field(field_s* next, char* field_name, char new_value);

/**
 * add a new field in the beginning of a field list
 * @param fields the fields list in which add the field (may be NULL to init a list)
 * @param field_name the name of the field to set
 * @param new_value the char* value to set, function will take care of malloc
 * @return new pointer to field_list
 */
field_s* add_string_field(field_s* next, char* field_name, char* new_value);

/**
 * add a new field in the beginning of a field list
 * @param fields the fields list in which add the field (may be NULL to init a list)
 * @param field_name the name of the field to set
 * @param new_value the float value to set, function will take care of malloc
 * @return new pointer to field_list
 */
field_s* add_float_field(field_s* next, char* field_name, float new_value);
/**
 * add a new field in the end of a field list
 * @param before the fields list at the end of which add the field
 * @param field_name the name of the field to set
 * @param new_value the value to set : a pointer to an ALREADY ALLOCATED space
 * @return new pointer to field_list
 */
field_s* add_field_end(field_s* before, char* field_name, void* new_value);

/**
 * set a field to a certain value
 * @param fields the fields list
 * @param field_name the name of the field to set
 * @param new_value the value to set : a pointer to an ALREADY ALLOCATED space
 * @param delete_old to precise if the old value must be freed (0 false, else true)
 * @return 1 if succes, 0 otherwise
 */
int set_field_value(field_s* fields, char* field_name, void* new_value, int delete_old);

/**
 * get a value for a certain field
 * @param fields the fields list
 * @param field_name the name of the field to qet
 * @return the value (need to cast after)
 */
void* get_field_value(field_s* fields, char* field_name);

/**
 * get name for a certain field
 * @param fields the fields list
 * @return the name
 */
char* get_field_name(field_s* fields);

/**
 * debug function to see the content of a field list
 * @param fields the fields list
 * @return void
 */

void view_fields(field_s* fields);

/**
 * debug function to see the content of a field list, with a certain function to show the content
 * @param fields the fields list
 * @param function to view the field content
 * @return void
 */

void view_fields_with_function(field_s* fields, void (*function) (void* field) );

/**
 * function to get the next field of a list
 * @param fields the fields list
 * @return the next element
 */
field_s* get_next_field(field_s* fields);

/**
 * check presence of a field (with its name) in a field list
 * @param field_name the name of the field to check
 * @param fields the fields list
 * @return the value : -1 if not found, 1 otherwise
 */

int check_presence(char* field_name,field_s* fields);

/**
 * check presence of a field list (from their name) in a field list
 * @param fields_names the name of the field to check
 * @param size the size of the array list
 * @param fields the fields list
 * @return the value : -1 if not at least one is not found, 1 otherwise
 */
int check_presence_list(char** fields_names, int size, field_s* fields);

/**
 * remove a field from the list of field
 * @param field the list of field
 * @param field_name the field to remove
 * @param with_content, indicate if content must be freed 0 false, else true
 * @return int 1 if succes, -1 otherwise
 */
int remove_field(field_s** fields, char* field_name, int with_content);

/**
 * free a list of fields (and their pointed values)
 * @param fields the fields list
 * @return void
 */
void free_all_fields(field_s* fields);

/**
 * free a list of fields (and their pointed values)
 * @param fields the fields list
 * @param the function to free the value
 * @return void
 */
void free_all_fields_with_function(field_s* fields, void (*free_func)(void* ele));
/**
 * free a field
 * @param fields the field
 * @param with_content indicates if the content of the field must be freed 0 false, else true
 * @return void
 */
void free_field(field_s* fields, int with_content);

/**
 * get the number of fields in  a list of fields
 * @param fields the field list
 * @return the number of fields
 */
int get_number_of_fields(field_s* fields);

#endif
