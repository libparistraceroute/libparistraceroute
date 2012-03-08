#include "output.h"
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

//fonctions associate_...() associates correct function with generic name depending on output and algo



output_s *output_base = NULL;

output_s* choose_output(char* output_name){
	if(output_name==NULL){
		return NULL;
	}
	output_s* current = output_base;
	char search = 1;
	while(search==1){
		if(current!=NULL && current->name_size!=strlen(output_name)){
			current=current->next;
		}
		else{
			if( current!=NULL && strncasecmp(current->name,output_name,current->name_size)!=0){
				current=current->next;
			}
			else{
				search=0;
			}
		}
	}
	if(current==NULL){
		#ifdef DEBUG
		fprintf(stderr,"choose_output : output %s not found in list\n",output_name);
		#endif
		return NULL;
	}
	return current;

}

void add_output_list (output_s *ops) {
	#ifdef DEBUG
	fprintf(stdout,"add_output_list : adding output %s to list\n",ops->name);
	#endif
	ops->next = output_base;
	output_base = ops;
}

/*
int set_printing_mode(output_s* output, char printing_mode){
	if(output==NULL){
		return -1;
	}
	output->printing_mode=printing_mode;
	return 1;
}*/
