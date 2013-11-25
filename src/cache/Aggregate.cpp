#include "Aggregate.h"

#include <stdio.h>

using namespace std;
using namespace rapidjson;

InternedStringContext  Aggregate::_buildIdStringCtx;

InternedStringContext  Aggregate::_revisionStringCtx;


void Aggregate::mergeJSON(const Value& dump) {
  const Value::Member* jvalues    = dump.FindMember("values");
  const Value::Member* jrevision  = dump.FindMember("revision");
  const Value::Member* jbuildId   = dump.FindMember("buildId");
  if (!jvalues || !jvalues->value.IsArray()) {
    fprintf(stderr, "'values' in dump isn't an array\n");
    return;
  }
  if (!jrevision || !jrevision->value.IsString()) {
    fprintf(stderr, "'revision' in dump isn't a string\n");
    return;
  }
  if (!jbuildId || !jbuildId->value.IsString()) {
    fprintf(stderr, "'buildId' in dump isn't a string\n");
    return;
  }
  const char* buildId  = jbuildId->value.GetString();
  const char* revision = jrevision->value.GetString();
  const Value& values  = jvalues->value;
  size_t length        = values.Size();

  // Check length of values
  if(length == 0) {
    fprintf(stderr, "Empty 'values' array in dump!\n");
  }

  // Check that we have doubles
  for (size_t i = 0; i < length; i++) {
    if(!values[i].IsNumber()) {
      fprintf(stderr, "Array contains non-double value!\n");
      return;
    }
  }

  // Check if length matches
  if (_length != length) {
    // Replace if we have newer buildId or current length is zero
    if (_length == 0 || _buildId < buildId) {
      // Set buildId and revision
      _buildId  = _buildIdStringCtx.createString(buildId);
      _revision = _revisionStringCtx.createString(revision);
      _length   = length;

      // Free old values
      if (_values) {
        delete[] _values;
      }
      _values = new double[length];

      for (size_t i = 0; i < length; i++) {
        _values[i] = values[i].GetDouble();
      }
    }
  } else {
    // Update revision and buildId if we have a newer one
    if (_buildId < buildId) {
      _buildId  = _buildIdStringCtx.createString(buildId);
      _revision = _revisionStringCtx.createString(revision);
    }

    size_t i;
    for (i = 0; i < length - 6; i++) {
      _values[i] += values[i].GetDouble();
    }
    for (; i < length - 1; i++) {
      double val = values[i].GetDouble();
      // Do not accumulate -1 (this indicates missing entry)
      if (val == -1 && _values[i] == -1) {
        continue;
      }
      _values[i] += val;
    }
    _values[i] += values[i].GetDouble();
  }
}

void Aggregate::output(OutputContext& ctx, PathNode<Aggregate>* owner) {
  if (ctx.comma) {
    fputc(',', ctx.file);
  }
  ctx.comma = true;
  fputc('\"', ctx.file);
  owner->output(ctx.file);
  fputs("\":{\"values\": [", ctx.file);
  if(_length > 0) {
    fprintf(ctx.file, "%.20g", _values[0]);
    for(size_t i = 1; i < _length; i++) {
      fprintf(ctx.file, ",%.20g", _values[i]);
    }
  }
  fprintf(ctx.file, "],\"buildId\":\"%s\",\"revision\":\"%s\"}",
          _buildId.data(), _revision.data());
}







