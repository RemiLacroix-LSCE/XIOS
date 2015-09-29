/* ************************************************************************** *
 *               Interface auto generated - do not modify                     *
 * ************************************************************************** */

#include <boost/multi_array.hpp>
#include <boost/shared_ptr.hpp>
#include "xios.hpp"
#include "attribute_template.hpp"
#include "object_template.hpp"
#include "group_template.hpp"
#include "icutil.hpp"
#include "icdate.hpp"
#include "timer.hpp"
#include "node_type.hpp"

extern "C"
{
  typedef xios::CFileGroup* filegroup_Ptr;

  void cxios_set_filegroup_append(filegroup_Ptr filegroup_hdl, bool append)
  {
    CTimer::get("XIOS").resume();
    filegroup_hdl->append.setValue(append);
    CTimer::get("XIOS").suspend();
  }

  void cxios_get_filegroup_append(filegroup_Ptr filegroup_hdl, bool* append)
  {
    CTimer::get("XIOS").resume();
    *append = filegroup_hdl->append.getInheritedValue();
    CTimer::get("XIOS").suspend();
  }

  bool cxios_is_defined_filegroup_append(filegroup_Ptr filegroup_hdl)
  {
     CTimer::get("XIOS").resume();
     bool isDefined = filegroup_hdl->append.hasInheritedValue();
     CTimer::get("XIOS").suspend();
     return isDefined;
  }


  void cxios_set_filegroup_compression_level(filegroup_Ptr filegroup_hdl, int compression_level)
  {
    CTimer::get("XIOS").resume();
    filegroup_hdl->compression_level.setValue(compression_level);
    CTimer::get("XIOS").suspend();
  }

  void cxios_get_filegroup_compression_level(filegroup_Ptr filegroup_hdl, int* compression_level)
  {
    CTimer::get("XIOS").resume();
    *compression_level = filegroup_hdl->compression_level.getInheritedValue();
    CTimer::get("XIOS").suspend();
  }

  bool cxios_is_defined_filegroup_compression_level(filegroup_Ptr filegroup_hdl)
  {
     CTimer::get("XIOS").resume();
     bool isDefined = filegroup_hdl->compression_level.hasInheritedValue();
     CTimer::get("XIOS").suspend();
     return isDefined;
  }


  void cxios_set_filegroup_description(filegroup_Ptr filegroup_hdl, const char * description, int description_size)
  {
    std::string description_str;
    if (!cstr2string(description, description_size, description_str)) return;
    CTimer::get("XIOS").resume();
    filegroup_hdl->description.setValue(description_str);
    CTimer::get("XIOS").suspend();
  }

  void cxios_get_filegroup_description(filegroup_Ptr filegroup_hdl, char * description, int description_size)
  {
    CTimer::get("XIOS").resume();
    if (!string_copy(filegroup_hdl->description.getInheritedValue(), description, description_size))
      ERROR("void cxios_get_filegroup_description(filegroup_Ptr filegroup_hdl, char * description, int description_size)", << "Input string is too short");
    CTimer::get("XIOS").suspend();
  }

  bool cxios_is_defined_filegroup_description(filegroup_Ptr filegroup_hdl)
  {
     CTimer::get("XIOS").resume();
     bool isDefined = filegroup_hdl->description.hasInheritedValue();
     CTimer::get("XIOS").suspend();
     return isDefined;
  }


  void cxios_set_filegroup_enabled(filegroup_Ptr filegroup_hdl, bool enabled)
  {
    CTimer::get("XIOS").resume();
    filegroup_hdl->enabled.setValue(enabled);
    CTimer::get("XIOS").suspend();
  }

  void cxios_get_filegroup_enabled(filegroup_Ptr filegroup_hdl, bool* enabled)
  {
    CTimer::get("XIOS").resume();
    *enabled = filegroup_hdl->enabled.getInheritedValue();
    CTimer::get("XIOS").suspend();
  }

  bool cxios_is_defined_filegroup_enabled(filegroup_Ptr filegroup_hdl)
  {
     CTimer::get("XIOS").resume();
     bool isDefined = filegroup_hdl->enabled.hasInheritedValue();
     CTimer::get("XIOS").suspend();
     return isDefined;
  }


  void cxios_set_filegroup_format(filegroup_Ptr filegroup_hdl, const char * format, int format_size)
  {
    std::string format_str;
    if (!cstr2string(format, format_size, format_str)) return;
    CTimer::get("XIOS").resume();
    filegroup_hdl->format.fromString(format_str);
    CTimer::get("XIOS").suspend();
  }

  void cxios_get_filegroup_format(filegroup_Ptr filegroup_hdl, char * format, int format_size)
  {
    CTimer::get("XIOS").resume();
    if (!string_copy(filegroup_hdl->format.getInheritedStringValue(), format, format_size))
      ERROR("void cxios_get_filegroup_format(filegroup_Ptr filegroup_hdl, char * format, int format_size)", << "Input string is too short");
    CTimer::get("XIOS").suspend();
  }

  bool cxios_is_defined_filegroup_format(filegroup_Ptr filegroup_hdl)
  {
     CTimer::get("XIOS").resume();
     bool isDefined = filegroup_hdl->format.hasInheritedValue();
     CTimer::get("XIOS").suspend();
     return isDefined;
  }


  void cxios_set_filegroup_group_ref(filegroup_Ptr filegroup_hdl, const char * group_ref, int group_ref_size)
  {
    std::string group_ref_str;
    if (!cstr2string(group_ref, group_ref_size, group_ref_str)) return;
    CTimer::get("XIOS").resume();
    filegroup_hdl->group_ref.setValue(group_ref_str);
    CTimer::get("XIOS").suspend();
  }

  void cxios_get_filegroup_group_ref(filegroup_Ptr filegroup_hdl, char * group_ref, int group_ref_size)
  {
    CTimer::get("XIOS").resume();
    if (!string_copy(filegroup_hdl->group_ref.getInheritedValue(), group_ref, group_ref_size))
      ERROR("void cxios_get_filegroup_group_ref(filegroup_Ptr filegroup_hdl, char * group_ref, int group_ref_size)", << "Input string is too short");
    CTimer::get("XIOS").suspend();
  }

  bool cxios_is_defined_filegroup_group_ref(filegroup_Ptr filegroup_hdl)
  {
     CTimer::get("XIOS").resume();
     bool isDefined = filegroup_hdl->group_ref.hasInheritedValue();
     CTimer::get("XIOS").suspend();
     return isDefined;
  }


  void cxios_set_filegroup_min_digits(filegroup_Ptr filegroup_hdl, int min_digits)
  {
    CTimer::get("XIOS").resume();
    filegroup_hdl->min_digits.setValue(min_digits);
    CTimer::get("XIOS").suspend();
  }

  void cxios_get_filegroup_min_digits(filegroup_Ptr filegroup_hdl, int* min_digits)
  {
    CTimer::get("XIOS").resume();
    *min_digits = filegroup_hdl->min_digits.getInheritedValue();
    CTimer::get("XIOS").suspend();
  }

  bool cxios_is_defined_filegroup_min_digits(filegroup_Ptr filegroup_hdl)
  {
     CTimer::get("XIOS").resume();
     bool isDefined = filegroup_hdl->min_digits.hasInheritedValue();
     CTimer::get("XIOS").suspend();
     return isDefined;
  }


  void cxios_set_filegroup_mode(filegroup_Ptr filegroup_hdl, const char * mode, int mode_size)
  {
    std::string mode_str;
    if (!cstr2string(mode, mode_size, mode_str)) return;
    CTimer::get("XIOS").resume();
    filegroup_hdl->mode.fromString(mode_str);
    CTimer::get("XIOS").suspend();
  }

  void cxios_get_filegroup_mode(filegroup_Ptr filegroup_hdl, char * mode, int mode_size)
  {
    CTimer::get("XIOS").resume();
    if (!string_copy(filegroup_hdl->mode.getInheritedStringValue(), mode, mode_size))
      ERROR("void cxios_get_filegroup_mode(filegroup_Ptr filegroup_hdl, char * mode, int mode_size)", << "Input string is too short");
    CTimer::get("XIOS").suspend();
  }

  bool cxios_is_defined_filegroup_mode(filegroup_Ptr filegroup_hdl)
  {
     CTimer::get("XIOS").resume();
     bool isDefined = filegroup_hdl->mode.hasInheritedValue();
     CTimer::get("XIOS").suspend();
     return isDefined;
  }


  void cxios_set_filegroup_name(filegroup_Ptr filegroup_hdl, const char * name, int name_size)
  {
    std::string name_str;
    if (!cstr2string(name, name_size, name_str)) return;
    CTimer::get("XIOS").resume();
    filegroup_hdl->name.setValue(name_str);
    CTimer::get("XIOS").suspend();
  }

  void cxios_get_filegroup_name(filegroup_Ptr filegroup_hdl, char * name, int name_size)
  {
    CTimer::get("XIOS").resume();
    if (!string_copy(filegroup_hdl->name.getInheritedValue(), name, name_size))
      ERROR("void cxios_get_filegroup_name(filegroup_Ptr filegroup_hdl, char * name, int name_size)", << "Input string is too short");
    CTimer::get("XIOS").suspend();
  }

  bool cxios_is_defined_filegroup_name(filegroup_Ptr filegroup_hdl)
  {
     CTimer::get("XIOS").resume();
     bool isDefined = filegroup_hdl->name.hasInheritedValue();
     CTimer::get("XIOS").suspend();
     return isDefined;
  }


  void cxios_set_filegroup_name_suffix(filegroup_Ptr filegroup_hdl, const char * name_suffix, int name_suffix_size)
  {
    std::string name_suffix_str;
    if (!cstr2string(name_suffix, name_suffix_size, name_suffix_str)) return;
    CTimer::get("XIOS").resume();
    filegroup_hdl->name_suffix.setValue(name_suffix_str);
    CTimer::get("XIOS").suspend();
  }

  void cxios_get_filegroup_name_suffix(filegroup_Ptr filegroup_hdl, char * name_suffix, int name_suffix_size)
  {
    CTimer::get("XIOS").resume();
    if (!string_copy(filegroup_hdl->name_suffix.getInheritedValue(), name_suffix, name_suffix_size))
      ERROR("void cxios_get_filegroup_name_suffix(filegroup_Ptr filegroup_hdl, char * name_suffix, int name_suffix_size)", << "Input string is too short");
    CTimer::get("XIOS").suspend();
  }

  bool cxios_is_defined_filegroup_name_suffix(filegroup_Ptr filegroup_hdl)
  {
     CTimer::get("XIOS").resume();
     bool isDefined = filegroup_hdl->name_suffix.hasInheritedValue();
     CTimer::get("XIOS").suspend();
     return isDefined;
  }


  void cxios_set_filegroup_output_freq(filegroup_Ptr filegroup_hdl, cxios_duration output_freq_c)
  {
    CTimer::get("XIOS").resume();
    filegroup_hdl->output_freq.allocate();
    CDuration& output_freq = filegroup_hdl->output_freq.get();
    output_freq.year = output_freq_c.year;
    output_freq.month = output_freq_c.month;
    output_freq.day = output_freq_c.day;
    output_freq.hour = output_freq_c.hour;
    output_freq.minute = output_freq_c.minute;
    output_freq.second = output_freq_c.second;
    output_freq.timestep = output_freq_c.timestep;
    CTimer::get("XIOS").suspend();
  }

  void cxios_get_filegroup_output_freq(filegroup_Ptr filegroup_hdl, cxios_duration* output_freq_c)
  {
    CTimer::get("XIOS").resume();
    CDuration output_freq = filegroup_hdl->output_freq.getInheritedValue();
    output_freq_c->year = output_freq.year;
    output_freq_c->month = output_freq.month;
    output_freq_c->day = output_freq.day;
    output_freq_c->hour = output_freq.hour;
    output_freq_c->minute = output_freq.minute;
    output_freq_c->second = output_freq.second;
    output_freq_c->timestep = output_freq.timestep;
    CTimer::get("XIOS").suspend();
  }

  bool cxios_is_defined_filegroup_output_freq(filegroup_Ptr filegroup_hdl)
  {
     CTimer::get("XIOS").resume();
     bool isDefined = filegroup_hdl->output_freq.hasInheritedValue();
     CTimer::get("XIOS").suspend();
     return isDefined;
  }


  void cxios_set_filegroup_output_level(filegroup_Ptr filegroup_hdl, int output_level)
  {
    CTimer::get("XIOS").resume();
    filegroup_hdl->output_level.setValue(output_level);
    CTimer::get("XIOS").suspend();
  }

  void cxios_get_filegroup_output_level(filegroup_Ptr filegroup_hdl, int* output_level)
  {
    CTimer::get("XIOS").resume();
    *output_level = filegroup_hdl->output_level.getInheritedValue();
    CTimer::get("XIOS").suspend();
  }

  bool cxios_is_defined_filegroup_output_level(filegroup_Ptr filegroup_hdl)
  {
     CTimer::get("XIOS").resume();
     bool isDefined = filegroup_hdl->output_level.hasInheritedValue();
     CTimer::get("XIOS").suspend();
     return isDefined;
  }


  void cxios_set_filegroup_par_access(filegroup_Ptr filegroup_hdl, const char * par_access, int par_access_size)
  {
    std::string par_access_str;
    if (!cstr2string(par_access, par_access_size, par_access_str)) return;
    CTimer::get("XIOS").resume();
    filegroup_hdl->par_access.fromString(par_access_str);
    CTimer::get("XIOS").suspend();
  }

  void cxios_get_filegroup_par_access(filegroup_Ptr filegroup_hdl, char * par_access, int par_access_size)
  {
    CTimer::get("XIOS").resume();
    if (!string_copy(filegroup_hdl->par_access.getInheritedStringValue(), par_access, par_access_size))
      ERROR("void cxios_get_filegroup_par_access(filegroup_Ptr filegroup_hdl, char * par_access, int par_access_size)", << "Input string is too short");
    CTimer::get("XIOS").suspend();
  }

  bool cxios_is_defined_filegroup_par_access(filegroup_Ptr filegroup_hdl)
  {
     CTimer::get("XIOS").resume();
     bool isDefined = filegroup_hdl->par_access.hasInheritedValue();
     CTimer::get("XIOS").suspend();
     return isDefined;
  }


  void cxios_set_filegroup_split_freq(filegroup_Ptr filegroup_hdl, cxios_duration split_freq_c)
  {
    CTimer::get("XIOS").resume();
    filegroup_hdl->split_freq.allocate();
    CDuration& split_freq = filegroup_hdl->split_freq.get();
    split_freq.year = split_freq_c.year;
    split_freq.month = split_freq_c.month;
    split_freq.day = split_freq_c.day;
    split_freq.hour = split_freq_c.hour;
    split_freq.minute = split_freq_c.minute;
    split_freq.second = split_freq_c.second;
    split_freq.timestep = split_freq_c.timestep;
    CTimer::get("XIOS").suspend();
  }

  void cxios_get_filegroup_split_freq(filegroup_Ptr filegroup_hdl, cxios_duration* split_freq_c)
  {
    CTimer::get("XIOS").resume();
    CDuration split_freq = filegroup_hdl->split_freq.getInheritedValue();
    split_freq_c->year = split_freq.year;
    split_freq_c->month = split_freq.month;
    split_freq_c->day = split_freq.day;
    split_freq_c->hour = split_freq.hour;
    split_freq_c->minute = split_freq.minute;
    split_freq_c->second = split_freq.second;
    split_freq_c->timestep = split_freq.timestep;
    CTimer::get("XIOS").suspend();
  }

  bool cxios_is_defined_filegroup_split_freq(filegroup_Ptr filegroup_hdl)
  {
     CTimer::get("XIOS").resume();
     bool isDefined = filegroup_hdl->split_freq.hasInheritedValue();
     CTimer::get("XIOS").suspend();
     return isDefined;
  }


  void cxios_set_filegroup_split_freq_format(filegroup_Ptr filegroup_hdl, const char * split_freq_format, int split_freq_format_size)
  {
    std::string split_freq_format_str;
    if (!cstr2string(split_freq_format, split_freq_format_size, split_freq_format_str)) return;
    CTimer::get("XIOS").resume();
    filegroup_hdl->split_freq_format.setValue(split_freq_format_str);
    CTimer::get("XIOS").suspend();
  }

  void cxios_get_filegroup_split_freq_format(filegroup_Ptr filegroup_hdl, char * split_freq_format, int split_freq_format_size)
  {
    CTimer::get("XIOS").resume();
    if (!string_copy(filegroup_hdl->split_freq_format.getInheritedValue(), split_freq_format, split_freq_format_size))
      ERROR("void cxios_get_filegroup_split_freq_format(filegroup_Ptr filegroup_hdl, char * split_freq_format, int split_freq_format_size)", << "Input string is too short");
    CTimer::get("XIOS").suspend();
  }

  bool cxios_is_defined_filegroup_split_freq_format(filegroup_Ptr filegroup_hdl)
  {
     CTimer::get("XIOS").resume();
     bool isDefined = filegroup_hdl->split_freq_format.hasInheritedValue();
     CTimer::get("XIOS").suspend();
     return isDefined;
  }


  void cxios_set_filegroup_sync_freq(filegroup_Ptr filegroup_hdl, cxios_duration sync_freq_c)
  {
    CTimer::get("XIOS").resume();
    filegroup_hdl->sync_freq.allocate();
    CDuration& sync_freq = filegroup_hdl->sync_freq.get();
    sync_freq.year = sync_freq_c.year;
    sync_freq.month = sync_freq_c.month;
    sync_freq.day = sync_freq_c.day;
    sync_freq.hour = sync_freq_c.hour;
    sync_freq.minute = sync_freq_c.minute;
    sync_freq.second = sync_freq_c.second;
    sync_freq.timestep = sync_freq_c.timestep;
    CTimer::get("XIOS").suspend();
  }

  void cxios_get_filegroup_sync_freq(filegroup_Ptr filegroup_hdl, cxios_duration* sync_freq_c)
  {
    CTimer::get("XIOS").resume();
    CDuration sync_freq = filegroup_hdl->sync_freq.getInheritedValue();
    sync_freq_c->year = sync_freq.year;
    sync_freq_c->month = sync_freq.month;
    sync_freq_c->day = sync_freq.day;
    sync_freq_c->hour = sync_freq.hour;
    sync_freq_c->minute = sync_freq.minute;
    sync_freq_c->second = sync_freq.second;
    sync_freq_c->timestep = sync_freq.timestep;
    CTimer::get("XIOS").suspend();
  }

  bool cxios_is_defined_filegroup_sync_freq(filegroup_Ptr filegroup_hdl)
  {
     CTimer::get("XIOS").resume();
     bool isDefined = filegroup_hdl->sync_freq.hasInheritedValue();
     CTimer::get("XIOS").suspend();
     return isDefined;
  }


  void cxios_set_filegroup_time_counter(filegroup_Ptr filegroup_hdl, const char * time_counter, int time_counter_size)
  {
    std::string time_counter_str;
    if (!cstr2string(time_counter, time_counter_size, time_counter_str)) return;
    CTimer::get("XIOS").resume();
    filegroup_hdl->time_counter.fromString(time_counter_str);
    CTimer::get("XIOS").suspend();
  }

  void cxios_get_filegroup_time_counter(filegroup_Ptr filegroup_hdl, char * time_counter, int time_counter_size)
  {
    CTimer::get("XIOS").resume();
    if (!string_copy(filegroup_hdl->time_counter.getInheritedStringValue(), time_counter, time_counter_size))
      ERROR("void cxios_get_filegroup_time_counter(filegroup_Ptr filegroup_hdl, char * time_counter, int time_counter_size)", << "Input string is too short");
    CTimer::get("XIOS").suspend();
  }

  bool cxios_is_defined_filegroup_time_counter(filegroup_Ptr filegroup_hdl)
  {
     CTimer::get("XIOS").resume();
     bool isDefined = filegroup_hdl->time_counter.hasInheritedValue();
     CTimer::get("XIOS").suspend();
     return isDefined;
  }


  void cxios_set_filegroup_timeseries(filegroup_Ptr filegroup_hdl, const char * timeseries, int timeseries_size)
  {
    std::string timeseries_str;
    if (!cstr2string(timeseries, timeseries_size, timeseries_str)) return;
    CTimer::get("XIOS").resume();
    filegroup_hdl->timeseries.fromString(timeseries_str);
    CTimer::get("XIOS").suspend();
  }

  void cxios_get_filegroup_timeseries(filegroup_Ptr filegroup_hdl, char * timeseries, int timeseries_size)
  {
    CTimer::get("XIOS").resume();
    if (!string_copy(filegroup_hdl->timeseries.getInheritedStringValue(), timeseries, timeseries_size))
      ERROR("void cxios_get_filegroup_timeseries(filegroup_Ptr filegroup_hdl, char * timeseries, int timeseries_size)", << "Input string is too short");
    CTimer::get("XIOS").suspend();
  }

  bool cxios_is_defined_filegroup_timeseries(filegroup_Ptr filegroup_hdl)
  {
     CTimer::get("XIOS").resume();
     bool isDefined = filegroup_hdl->timeseries.hasInheritedValue();
     CTimer::get("XIOS").suspend();
     return isDefined;
  }


  void cxios_set_filegroup_ts_prefix(filegroup_Ptr filegroup_hdl, const char * ts_prefix, int ts_prefix_size)
  {
    std::string ts_prefix_str;
    if (!cstr2string(ts_prefix, ts_prefix_size, ts_prefix_str)) return;
    CTimer::get("XIOS").resume();
    filegroup_hdl->ts_prefix.setValue(ts_prefix_str);
    CTimer::get("XIOS").suspend();
  }

  void cxios_get_filegroup_ts_prefix(filegroup_Ptr filegroup_hdl, char * ts_prefix, int ts_prefix_size)
  {
    CTimer::get("XIOS").resume();
    if (!string_copy(filegroup_hdl->ts_prefix.getInheritedValue(), ts_prefix, ts_prefix_size))
      ERROR("void cxios_get_filegroup_ts_prefix(filegroup_Ptr filegroup_hdl, char * ts_prefix, int ts_prefix_size)", << "Input string is too short");
    CTimer::get("XIOS").suspend();
  }

  bool cxios_is_defined_filegroup_ts_prefix(filegroup_Ptr filegroup_hdl)
  {
     CTimer::get("XIOS").resume();
     bool isDefined = filegroup_hdl->ts_prefix.hasInheritedValue();
     CTimer::get("XIOS").suspend();
     return isDefined;
  }


  void cxios_set_filegroup_type(filegroup_Ptr filegroup_hdl, const char * type, int type_size)
  {
    std::string type_str;
    if (!cstr2string(type, type_size, type_str)) return;
    CTimer::get("XIOS").resume();
    filegroup_hdl->type.fromString(type_str);
    CTimer::get("XIOS").suspend();
  }

  void cxios_get_filegroup_type(filegroup_Ptr filegroup_hdl, char * type, int type_size)
  {
    CTimer::get("XIOS").resume();
    if (!string_copy(filegroup_hdl->type.getInheritedStringValue(), type, type_size))
      ERROR("void cxios_get_filegroup_type(filegroup_Ptr filegroup_hdl, char * type, int type_size)", << "Input string is too short");
    CTimer::get("XIOS").suspend();
  }

  bool cxios_is_defined_filegroup_type(filegroup_Ptr filegroup_hdl)
  {
     CTimer::get("XIOS").resume();
     bool isDefined = filegroup_hdl->type.hasInheritedValue();
     CTimer::get("XIOS").suspend();
     return isDefined;
  }
}
