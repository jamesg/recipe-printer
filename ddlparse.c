#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifndef max
	#define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#define STREAM_DEBUG stdout

struct list_t {
	struct ingredient_t* ingredient;
	struct list_t* next;
};

struct ingredient_t {
	struct list_t* ingredient_list;
	char* label;
	int column;
};

struct ingredient_t* dish;

void* malloc_chk(size_t size) {
	void* v = 0;
	if ((v = malloc(size)) == 0) {
		fprintf(STREAM_DEBUG, "Error allocating memory\n");
		exit(1);
	}
	return v;
}

// Allocate memory for a list item and add it to list head
// If the list is a stack, this is like pushing the item.
struct list_t* malloc_list_item(struct list_t** head) {
	struct list_t* list = malloc_chk(sizeof(struct list_t));
	(*list).ingredient = 0;
	if ((*head) != 0) {
		(*list).next = *head;
		*head = list;
	} else {
		(*list).next = 0;
	}
	return list;
}

// Allocate memory for an ingredient and add it to parent
struct ingredient_t* malloc_ingredient(struct ingredient_t* parent) {
	struct ingredient_t* new_ingredient = malloc_chk(sizeof(struct ingredient_t));
	(*new_ingredient).label = 0;
	(*new_ingredient).ingredient_list = 0;
	(*new_ingredient).column = 0;

	if (parent != 0) {
		struct list_t* new_list_item = malloc_chk(sizeof(struct list_t));
		(*new_list_item).ingredient = new_ingredient;
		(*new_list_item).next = (*parent).ingredient_list;
		(*parent).ingredient_list = new_list_item;
	}

	return new_ingredient;
}

// Get the first item in the list of sub ingredients
struct ingredient_t* head(struct ingredient_t* ingredient) {
	return (*(*ingredient).ingredient_list).ingredient;
}

// Free one list item.  If the list is a stack, this is like popping the item.
struct ingredient_t* free_list_item(struct list_t** l) {
	if (*l == 0) return 0;

	struct ingredient_t* i;
	i = (**l).ingredient;

	struct list_t* t = (*(*l)).next;
	free(*l);
	(*l) = t;

	return i;
}

// Free the list of ingredients, not any strings
void free_ingredient(struct ingredient_t* ingredient) {
	struct list_t* i = (*ingredient).ingredient_list;
	while (i != 0) {
		struct list_t* n = (*i).next;
		if ((*i).ingredient != 0) {
			free_ingredient((*i).ingredient);
		}
		i = n;
	}
}

void print_list(struct list_t* l) {
	printf("list <");
	while (l != 0) {
		printf("%s, ", (*(*l).ingredient).label);
		l = (*l).next;
	}
	printf(">\n");
}

void ddlparse(struct ingredient_t* ingredient, char** buffer) {
	enum {
		NEW_INGREDIENT, NEW_LISTPART, NEW_METHOD,
		INGREDIENT, LISTPART, METHOD
	} state = NEW_INGREDIENT;

	while (**buffer != '\0') {
		char c = **buffer;
		switch (state) {
		case NEW_INGREDIENT:
			switch (c) {
			// Compound ingredient
			case '(': {
				(char*)(*buffer)++;
				struct ingredient_t* new_ingredient = malloc_ingredient(ingredient);
				ddlparse(new_ingredient, buffer);
				break;
			}
			case '>':
			case ',':
				state = NEW_LISTPART;
				break;
			// Single ingredient
			default: {
				struct ingredient_t* new_ingredient = malloc_ingredient(ingredient);
				(*new_ingredient).label = (char*)(*buffer);
				(char*)(*buffer)++;
				state = INGREDIENT;
				break;
				}
			}
			break;
		case INGREDIENT:
			switch (c) {
			case ',':
			case '>':
				state = NEW_LISTPART;
				break;
			default:
				(char*)(*buffer)++;
				break;
			}
			break;
		case NEW_LISTPART:
			switch (c) {
			case ',':
				state = NEW_INGREDIENT;
				break;
			case '>':
				state = NEW_METHOD;
				break;
			}
			**buffer = '\0';
			(char*)(*buffer)++;
			break;
		case NEW_METHOD:
			(*ingredient).label = (char*)(*buffer);
			(char*)(*buffer)++;
			state = METHOD;
			break;
		case METHOD:
			switch (c) {
			case ')': {
				(**buffer) = '\0';
				(char*)(*buffer)++;
				return;
				break;
				}
			case '\n':
				(**buffer) = '\0';
				(char*)(*buffer)++;
				break;
			default:
				(char*)(*buffer)++;
				break;
			}
			break;
		default:
			printf("Should never be reached\n");
			break;
		}
	}
}

void print_ingredient(struct ingredient_t* ingredient) {
	if (ingredient == 0) return;
	if ((*ingredient).ingredient_list != 0) { // there are sub ingredients
		printf("c%d (", (*ingredient).column);
		// print all the sub-ingredients
		struct list_t* i = (*ingredient).ingredient_list;
		while (i != 0) {
			print_ingredient((*i).ingredient);
			if ((*i).next != 0) {
				printf(",");
			}
			i = (*i).next;
		}
		// print the label
		printf(">%s", (*ingredient).label);
		printf(")");
	} else {
		// just print the label
		printf((*ingredient).label);
	}
}

void printn(char* s, int n) {
	int i = printf(s);
	for (; i < n; i++) {
		printf(" ");
	}
}

int max_depth(struct ingredient_t* ingredient) {
	int depth = 1;

	struct list_t* i = (*ingredient).ingredient_list;
	while (i != 0) {
		depth = max(depth, max_depth((*i).ingredient)+1);
		i = (*i).next;
	}
	return depth;
}

// Assign column numbers to ingredients
int assign_columns(struct ingredient_t* ingredient) {
	struct list_t* i = (*ingredient).ingredient_list;

	while (i != 0) {
		int t = assign_columns((*i).ingredient);
		(*ingredient).column = max((*ingredient).column, t+1);
		i = (*i).next;
	}
	return (*ingredient).column;
}

void print_recipe(struct ingredient_t* ingredient) {
	struct list_t* i = (*ingredient).ingredient_list;

	while (i != 0) {
		if ((*i).next != 0) {
			print_recipe((*i).ingredient);
			printn((*((*i).ingredient)).label,20);
			printf("\n");
		} else {
			print_recipe((*i).ingredient);
			printn((*((*i).ingredient)).label,20);
		}
		i = (*i).next;
	}
}

int main(int argc, char* args[]) {
	if (argc != 2) {
		fprintf(STREAM_DEBUG, "Usage: ddlparse filename\n");
		exit(1);
	}
	struct stat stat_d;
	if ((stat(args[1], &stat_d)) != 0) {
		fprintf(STREAM_DEBUG, "File not found\n");
		exit(1);
	}

	char* buffer = 0;
	if ((buffer = malloc((stat_d.st_blocks*stat_d.st_blksize)+1)) == 0) {
		fprintf(STREAM_DEBUG, "Could not allocate memory\n");
		exit(1);
	}

	FILE* file = fopen(args[1], "r");
	long filesize = fread(buffer,sizeof(char),(stat_d.st_blocks*stat_d.st_blksize),file);

	buffer[filesize] = 0;

	char* working_buffer = buffer;
	struct ingredient_t* dish = malloc_ingredient(0);

	ddlparse(dish, &working_buffer);

	assign_columns(dish);
	print_ingredient(dish);
	printf("\n");
	print_recipe(dish);
	printf("\n");
	
	return 0;
}

