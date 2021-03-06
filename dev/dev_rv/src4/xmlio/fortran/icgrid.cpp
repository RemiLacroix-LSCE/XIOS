/* ************************************************************************** *
 *      Copyright © IPSL/LSCE, XMLIOServer, Avril 2010 - Octobre 2011         *
 * ************************************************************************** */

#include "icutil.hpp"

extern "C"
{
// /////////////////////////////// Définitions ////////////////////////////// //

   // ----------------------- Redéfinition de types ----------------------------
   
   typedef void * XGridPtr, * XGridGroupPtr;

   // ------------------------- Attributs des axes -----------------------------
   
   void xios_set_grid_name(XGridPtr grid_hdl, const char * name,  int name_size)
   {
      std::string name_str; 
      if (!cstr2string(name, name_size, name_str)) return; 
   }
   
   void xios_set_grid_description(XGridPtr grid_hdl, const char * description,  int description_size)
   {
      std::string description_str; 
      if (!cstr2string(description, description_size, description_str)) return;
   }
   
   void xios_set_grid_domain_ref(XGridPtr grid_hdl, const char * domain_ref,  int domain_ref_size)
   {
      std::string domain_ref_str; 
      if (!cstr2string(domain_ref, domain_ref_size, domain_ref_str)) return; 
   }
   
   void xios_set_grid_axis_ref(XGridPtr grid_hdl, const char * axis_ref,  int axis_ref_size)
   {
      std::string axis_ref_str; 
      if (!cstr2string(axis_ref, axis_ref_size, axis_ref_str)) return; 
   }
   
   // -------------------- Attributs des groupes de grilles --------------------
   
   void xios_set_gridgroup_name(XGridGroupPtr gridgroup_hdl, const char * name,  int name_size)
   {
      std::string name_str; 
      if (!cstr2string(name, name_size, name_str)) return; 
   }
   
   void xios_set_gridgroup_description(XGridGroupPtr gridgroup_hdl, const char * description,  int description_size)
   {
      std::string description_str; 
      if (!cstr2string(description, description_size, description_str)) return;
   }
   
   void xios_set_gridgroup_domain_ref(XGridGroupPtr gridgroup_hdl, const char * domain_ref,  int domain_ref_size)
   {
      std::string domain_ref_str; 
      if (!cstr2string(domain_ref, domain_ref_size, domain_ref_str)) return; 
   }
   
   void xios_set_gridgroup_axis_ref(XGridGroupPtr gridgroup_hdl, const char * axis_ref,  int axis_ref_size)
   {
      std::string axis_ref_str; 
      if (!cstr2string(axis_ref, axis_ref_size, axis_ref_str)) return; 
   }
   
   // ------------------------ Création des handle -----------------------------
  
   void xios_grid_handle_create (XGridPtr * _ret, const char * _id, int _id_len)
   {
      std::string id; 
      if (!cstr2string(_id, _id_len, id)) return;
   }
   
   void xios_gridgroup_handle_create (XGridGroupPtr * _ret, const char * _id, int _id_len)
   {
      std::string id; 
      if (!cstr2string(_id, _id_len, id)) return;
   }
  
   
} // extern "C"
