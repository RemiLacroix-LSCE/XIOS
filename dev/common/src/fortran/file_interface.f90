MODULE FILE_INTERFACE
   USE, INTRINSIC :: ISO_C_BINDING
   
   INTERFACE ! Ne pas appeler directement/Interface FORTRAN 2003 <-> C99
   
      SUBROUTINE cxios_set_file_name(file_hdl, name, name_size) BIND(C)
         USE ISO_C_BINDING
         INTEGER  (kind = C_INTPTR_T), VALUE        :: file_hdl
         CHARACTER(kind = C_CHAR)    , DIMENSION(*) :: name
         INTEGER  (kind = C_INT)     , VALUE        :: name_size
      END SUBROUTINE cxios_set_file_name

      SUBROUTINE cxios_set_file_description(file_hdl, description, description_size) BIND(C)
         USE ISO_C_BINDING
         INTEGER  (kind = C_INTPTR_T), VALUE        :: file_hdl
         CHARACTER(kind = C_CHAR)    , DIMENSION(*) :: description
         INTEGER  (kind = C_INT)     , VALUE        :: description_size
      END SUBROUTINE cxios_set_file_description
      
      SUBROUTINE cxios_set_file_name_suffix(file_hdl, name_suffix, name_suffix_size) BIND(C)
         USE ISO_C_BINDING
         INTEGER  (kind = C_INTPTR_T), VALUE        :: file_hdl
         CHARACTER(kind = C_CHAR)    , DIMENSION(*) :: name_suffix
         INTEGER  (kind = C_INT)     , VALUE        :: name_suffix_size
      END SUBROUTINE cxios_set_file_name_suffix

      SUBROUTINE cxios_set_file_output_freq(file_hdl, output_freq, output_freq_size) BIND(C)
         USE ISO_C_BINDING
         INTEGER  (kind = C_INTPTR_T), VALUE        :: file_hdl
         CHARACTER(kind = C_CHAR)    , DIMENSION(*) :: output_freq
         INTEGER  (kind = C_INT)     , VALUE        :: output_freq_size
      END SUBROUTINE cxios_set_file_output_freq

      SUBROUTINE cxios_set_file_output_level(file_hdl, output_level) BIND(C)
         USE ISO_C_BINDING
         INTEGER (kind = C_INTPTR_T), VALUE :: file_hdl
         INTEGER (kind = C_INT)     , VALUE :: output_level
      END SUBROUTINE cxios_set_file_output_level

      SUBROUTINE cxios_set_file_enabled(file_hdl, enabled) BIND(C)
         USE ISO_C_BINDING
         INTEGER (kind = C_INTPTR_T), VALUE :: file_hdl
         LOGICAL (kind = C_BOOL)    , VALUE :: enabled
      END SUBROUTINE cxios_set_file_enabled
   
      SUBROUTINE cxios_file_handle_create(ret, idt, idt_size) BIND(C)
         USE ISO_C_BINDING
         INTEGER  (kind = C_INTPTR_T)               :: ret
         CHARACTER(kind = C_CHAR)    , DIMENSION(*) :: idt
         INTEGER  (kind = C_INT)     , VALUE        :: idt_size
      END SUBROUTINE cxios_file_handle_create

      SUBROUTINE cxios_file_valid_id(ret, idt, idt_size) BIND(C)
         USE ISO_C_BINDING
         LOGICAL  (kind = C_BOOL)                   :: ret
         CHARACTER(kind = C_CHAR)    , DIMENSION(*) :: idt
         INTEGER  (kind = C_INT)     , VALUE        :: idt_size
      END SUBROUTINE cxios_file_valid_id

     SUBROUTINE cxios_set_file_type(file_hdl, type, type_size) BIND(C)
         USE ISO_C_BINDING
         INTEGER  (kind = C_INTPTR_T), VALUE        :: file_hdl
         CHARACTER(kind = C_CHAR)    , DIMENSION(*) :: type
         INTEGER  (kind = C_INT)     , VALUE        :: type_size
      END SUBROUTINE cxios_set_file_type
      
   END INTERFACE
   
END MODULE FILE_INTERFACE
