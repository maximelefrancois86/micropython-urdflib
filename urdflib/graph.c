#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "py/runtime.h"
#include "py/obj.h"
#include "py/objstr.h"

#include "globals.h"
#include "graph.h"
#include "terms.h"

STATIC mp_obj_t graph_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    // check the number of arguments
    mp_arg_check_num(n_args, n_kw, 0, 1, true);

    graph_obj_t *self = m_new_obj(graph_obj_t);
    self->base.type = &urdflib_graph_type;
    urdflib_globals_init0();

    if (strcmp(mp_obj_get_type_str(args[0]), "bool") == 0)
    {
        bnode_obj_t *_bnode = m_new_obj(bnode_obj_t);
        _bnode->base.type = &urdflib_bnode_type;
        _bnode->bnode = middleware_terms_bnode_new(_generateRandomString(16));
        self->graph = middleware_graph_graph_new(_bnode->bnode->node);
    }
    else
    {
        if (mp_obj_is_type(args[0], &urdflib_uriref_type))
        {
            uriref_obj_t *_uriref = MP_OBJ_TO_PTR(args[0]);
            self->graph = middleware_graph_graph_new(_uriref->uri_ref->node);
        }
        else if (mp_obj_is_type(args[0], &urdflib_bnode_type))
        {
            bnode_obj_t *_bnode = MP_OBJ_TO_PTR(args[0]);
            self->graph = middleware_graph_graph_new(_bnode->bnode->node);
        }
        else
        {
            mp_raise_TypeError("The optional argument must be a URIRef or BNode");
        }
    }
    return MP_OBJ_FROM_PTR(self);
}

STATIC void graph_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    (void)kind;
    graph_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_print_str(print, "URDFLib;Graph(");
    mp_print_str(print, (const char *)sord_node_get_string(self->graph->context));
    mp_print_str(print, ")");
    // SordIter *iter = sord_begin(self->graph->model->model);
    // for (; !sord_iter_end(iter); sord_iter_next(iter))
    // {
    //     SordQuad quad = {NULL, NULL, NULL, NULL};
    //     sord_iter_get(iter, quad);
    //     if (sord_node_equals(quad[3], self->graph->node))
    //     {
    //         printf("<%s> <%s> <%s> <%s>\n", sord_node_get_string(quad[0]), sord_node_get_string(quad[1]), sord_node_get_string(quad[2]), sord_node_get_string(quad[3]));
    //     }
    // }
    // sord_iter_free(iter);
}

STATIC mp_obj_t graph_len(mp_obj_t self_in)
{
    graph_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_int(middleware_graph_num_quads(self->graph));
}
MP_DEFINE_CONST_FUN_OBJ_1(graph_len_obj, graph_len);

STATIC mp_obj_t graph_close(mp_obj_t self_in)
{
    graph_obj_t *self = MP_OBJ_TO_PTR(self_in);
    middleware_graph_close(self->graph);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(graph_close_obj, graph_close);

SordNode** _extractTriple(mp_obj_tuple_t *triple_in)
{
    SordNode** nodes = malloc(3 * sizeof(SordNode*));
    for (int8_t i = 0; i < 3; i++)
    {
        if (strcmp(mp_obj_get_type_str(triple_in->items[i]), "URIRef") != 0)
        {
            uriref_obj_t *_uriref = MP_OBJ_TO_PTR(triple_in->items[i]);
            nodes[i] = _uriref->uri_ref->node;
        }
        else if (strcmp(mp_obj_get_type_str(triple_in->items[i]), "BNode") != 0)
        {
            bnode_obj_t *_bnode = MP_OBJ_TO_PTR(triple_in->items[i]);
            nodes[i] = _bnode->bnode->node;
        }
        else if (strcmp(mp_obj_get_type_str(triple_in->items[i]), "Literal") != 0)
        {
            literal_obj_t *_literal = MP_OBJ_TO_PTR(triple_in->items[i]);
            nodes[i] = _literal->literal->node;
        }
        else
        {
            mp_raise_ValueError("Wrong type of subject");
        }
    }
    return nodes;
}

STATIC mp_obj_t graph_add(mp_obj_t self_in, mp_obj_t triple_in)
{
    graph_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_tuple_t *triple = MP_OBJ_TO_PTR(triple_in);
    if (triple->len != 3)
    {
        mp_raise_ValueError("Triple must be a tuple of length 3");
    }

    SordNode** nodes= _extractTriple(triple);
    if (middleware_graph_add(self->graph, nodes[0], nodes[1], nodes[2]))
    {
        mp_print_str(&mp_plat_print, "Added to the graph\n");
    }
    else
    {
        mp_print_str(&mp_plat_print, "Failed to add to the graph\n");
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(graph_add_obj, graph_add);

STATIC mp_obj_t graph_remove(mp_obj_t self_in, mp_obj_t triple_in)
{
    graph_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_tuple_t *triple = MP_OBJ_TO_PTR(triple_in);
    if (triple->len != 3)
    {
        mp_raise_ValueError("Triple must be a tuple of length 3");
    }
    SordNode** nodes= _extractTriple(triple);
    middleware_graph_remove(self->graph, nodes[0], nodes[1], nodes[2]);
    
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(graph_remove_obj, graph_remove);

// STATIC mp_obj_t graph_subjects(mp_obj_t self_in)
// {
//     // graph_obj_t *self = MP_OBJ_TO_PTR(self_in);
//     return mp_const_none;
// }

STATIC const mp_rom_map_elem_t graph_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR_length), MP_ROM_PTR(&graph_len_obj)},
    {MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&graph_close_obj)},
    {MP_ROM_QSTR(MP_QSTR_add), MP_ROM_PTR(&graph_add_obj)},
    {MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&graph_remove_obj)},

    // {MP_ROM_QSTR(MP_QSTR_subjects), MP_ROM_PTR(&graph_subjects_obj)},
};
STATIC MP_DEFINE_CONST_DICT(graph_locals_dict, graph_locals_dict_table);

const mp_obj_type_t urdflib_graph_type = {
    {&mp_type_type},
    .name = MP_QSTR_Graph,
    .print = graph_print,
    .make_new = graph_make_new,
    .locals_dict = (mp_obj_dict_t *)&graph_locals_dict,
};