#ifndef __XPATH_VERIFY_H__
#define __XPATH_VERIFY_H__

#include "jsonpath_model.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PredicateCondition{
    char* name;
    char* text;
} PredicateCondition;


typedef enum JSONPathValueType {
    jvt_null = 0, jvt_number, jvt_string, jvt_boolean
} JSONPathValueType;

typedef struct JSONPathValue {
    JSONPathValueType vtype;
    union {
        double number;
        char* string;
        bool boolean;
    };
} JSONPathValue;

typedef struct JSONPathKeyValuePair {
    char* key;
    JSONPathValue value;
    int state;
} JSONPathKeyValuePair;




static inline JSONPathValue findValueInTable(const char* name, JSONPathKeyValuePair* table) {
    int i = 0;
    while (table[i].key != NULL) {
        int r = strcmp(table[i].key, name);
        if (r == 0) {
            return table[i].value;
        }
        i++;
    }
    JSONPathValue v = {jvt_null};
    return v;
}

static inline JSONPathValue computeTypeCast(JSONPathValue v) {
    if (v.vtype == jvt_null) v.boolean = false;
    if (v.vtype == jvt_number) v.boolean = (v.number != 0.0);
    if (v.vtype == jvt_string) v.boolean = (v.string != NULL);
    v.vtype = jvt_boolean;
    return v;
}


static inline JSONPathValue computeUnaryOperator(JSONPathOperatorType opt, JSONPathValue v1) {
    JSONPathValue v = {jvt_null};
    switch (opt)
    {
        case jot_not:
            v1 = computeTypeCast(v1);
            v.vtype = jvt_boolean;
            v.boolean = !(v1.boolean);
            break;
    
        default:
            break;
    }
    return v;
}

/**
    jot_or = 0, jot_and, // boolean
    jot_equal, jot_less, jot_greater, jot_neq, jot_leq, jot_geq,  // comparing
    jot_add, jot_minus, jot_multiply, jot_div, jot_mod, // calculation
    jot_union // node-set union
*/

static inline JSONPathValue computeBinaryOperator(JSONPathOperatorType opt, JSONPathValue v1, JSONPathValue v2) {
    JSONPathValue v;
    // type casting
    if (opt == jot_or || opt == jot_and) {
        v1 = computeTypeCast(v1);
        v2 = computeTypeCast(v2);
    }
    // calculate 
    switch (opt) {
        case jot_or:
            v.vtype = jvt_boolean;
            v.boolean = v1.boolean || v2.boolean;
            return v;
        case jot_and:
            v.vtype = jvt_boolean;
            v.boolean = v1.boolean && v2.boolean;
            return v;
        case jot_equal:
            v.vtype = jvt_boolean;
            v.boolean = false;
            if (v1.vtype == v2.vtype && v1.vtype == jvt_boolean)
                v.boolean = (v1.boolean == v2.boolean);
            else if (v1.vtype == v2.vtype && v1.vtype == jvt_number)
                v.boolean = (v1.number == v2.number);
            else if (v1.vtype == v2.vtype && v1.vtype == jvt_string) {
                int r = strcmp(v1.string, v2.string);
                v.boolean = (r == 0);
            } else if (v1.vtype == v2.vtype && v1.vtype == jvt_null) {
                v.boolean = true;
            }
            return v;
        case jot_neq:
            v = computeBinaryOperator(jot_equal, v1, v2);
            v.boolean = !(v.boolean);
            return v;
        case jot_less:
            v.vtype = jvt_boolean;
            v.boolean = (v1.number < v2.number);
            return v;
        case jot_greater:
            v.vtype = jvt_boolean;
            v.boolean = (v1.number > v2.number);
            return v;
        case jot_leq:
            v.vtype = jvt_boolean;
            v.boolean = (v1.number <= v2.number);
            return v;
        case jot_geq:
            v.vtype = jvt_boolean;
            v.boolean = (v1.number >= v2.number);
            return v;
        case jot_add:
            v.vtype = jvt_number;
            v.number = (v1.number + v2.number);
            return v;
        case jot_minus:
            v.vtype = jvt_number;
            v.number = (v1.number - v2.number);
            return v;
        case jot_multiply:
            v.vtype = jvt_number;
            v.number = (v1.number * v2.number);
            return v;
        case jot_div:
            v.vtype = jvt_number;
            v.number = (v1.number / v2.number);
            return v;
        case jot_mod:
            v.vtype = jvt_number;
            v.number = (long long int)(v1.number) % (long long int)(v2.number);
            return v;
    }
    return v;
}

static inline JSONPathValue stringToValue(const char* str) {
    JSONPathValue v = {.vtype = jvt_null};
    if (str == NULL) return v;
    if (strcmp(str, "true") == 0) {
        v.vtype = jvt_boolean; v.boolean = true;
        return v;
    }
    if (strcmp(str, "false") == 0) {
        v.vtype = jvt_boolean; v.boolean = false;
        return v;
    }
    if (strcmp(str, "null") == 0) {
        v.vtype = jvt_null; 
        return v;
    }
    int len = strlen(str);
    if (len == 0) {
        v.vtype = jvt_boolean; v.boolean = true;
        return v;
    }
    if (str[0] == '"') { 
        char* s = (char*) malloc(sizeof(char) * (len-1));
        strncpy(s, str+1, len-2);
        s[len-2] = '\0';
        v.vtype = jvt_string; v.string = s;
        return v;
    }
    if (isdigit(str[0])) {
        v.vtype = jvt_number; v.number = atof(str);
        return v;
    }
    return v;
} 

static inline JSONPathKeyValuePair* createTableFromConditions(PredicateCondition* conditions) {
    int length = 0;
    while (conditions[length].name != NULL) length++;

    JSONPathKeyValuePair* table = (JSONPathKeyValuePair*) calloc(sizeof(JSONPathKeyValuePair), length+1);
    for (int i = 0; i < length; ++i) {
        table[i].key = conditions[i].name;
        table[i].value = stringToValue(conditions[i].text);
    }
    return table;
}

static inline void clearPredicateCondtion(PredicateCondition* table) {
    int i = 0;
    while (table[i].name != NULL) {
        if (table[i].text) free(table[i].text);
        table[i].text = NULL;
        i++;
    }
}

static inline JSONPathValue calculateExpression(ASTNode* node, JSONPathKeyValuePair* table) {
     switch (node->node_type) {
        case jnt_operator: {
            JSONPathValue v1, v2;
            v1 = calculateExpression(node->left, table);
            if (node->right) {
                v2 = calculateExpression(node->right, table);
                return computeBinaryOperator(node->opt, v1, v2);
            }
            return computeUnaryOperator(node->opt, v1);
        }
        case jnt_variable: {
            JSONPathValue v = findValueInTable(node->string, table);
            return v;
        }
        case jnt_string: {
            JSONPathValue v = {.vtype = jvt_string};
            v.string = node->string;
            return v;
        }
        case jnt_number: {
            JSONPathValue v = {.vtype = jvt_number};
            v.number = node->number;
            return v;
        }
        default: {
            JSONPathValue v = {jvt_null};
            return v;
        }
    }
}


/**
 * Before calling this function, please replace the '@.proptery' subtree to a single node - jnt_variable
 * You can create it by calling @ASTNodeCreateVariable(var_name)
 * When I calculate the result, I will try to search the table to find the values of those variable nodes.
 */
static inline bool evaluateExpression(ASTNode* node, PredicateCondition* conditions) {
    JSONPathKeyValuePair* table = createTableFromConditions(conditions);
    JSONPathValue v = calculateExpression(node, table);
    return computeTypeCast(v).boolean;
}



static inline ASTNode* evaluatorModifyReference(ASTNode* node) {
    ASTNode* ret = 0;
    if (node->left) {
        ret = evaluatorModifyReference(node->left);
        if (ret != NULL) node->left = ret;
    }
    if (node->right) {
        ret = evaluatorModifyReference(node->right);
        if (ret != NULL) node->right = ret;
    }
    if (node->node_type == jnt_reference) {
        return NULL;
    }
    if (node->node_type == jnt_variable) {
        return NULL;
    }
    if (node->node_type == jnt_concat) {
        if (node->left->node_type == jnt_reference) {
            char* name = (char*) malloc(sizeof(char) * (strlen(node->right->string) + 1));
            strcpy(name, node->right->string);
            ASTNode* new_node = ASTNodeCreateVariable(name);
            free(node->left);
            free(node->right->string);
            free(node->right);
            free(node);
            return new_node;
        } 
        if (node->left->node_type == jnt_variable) {
            char* name = (char*) malloc(sizeof(char) * (
                strlen(node->right->string) + strlen(node->left->string) + 2));
            strcpy(name, node->left->string);
            strcat(name, ".");
            strcat(name, node->right->string);
            free(node->left->string);
            free(node->right->string);
            free(node->right);
            free(node);
            node->left->string = name;
            return node->left;
        }
    }
    return NULL;
}





#ifdef __cplusplus
}
#endif


#endif