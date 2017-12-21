/**
 * @file  tdbpp_arraymeta.h
 *
 * @author Ravi Gaddipati
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 TileDB, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * This file declares the C++ API for TileDB.
 */

#ifndef TILEDB_TDBPP_ARRAYMETA_H
#define TILEDB_TDBPP_ARRAYMETA_H

#include "tiledb.h"
#include "tdbpp_object.h"
#include "tdbpp_attribute.h"
#include "tdbpp_domain.h"
#include "tdbpp_context.h"

#include <memory>
#include <unordered_map>
#include <functional>
#include <vector>

namespace tdb {

  class ArrayMetadata {
  public:
    ArrayMetadata(Context &ctx) : _ctx(ctx), _deleter(ctx) {};
    ArrayMetadata(Context &ctx, tiledb_array_metadata_t **meta) : ArrayMetadata(ctx) {
      if (meta && *meta) {
        _init(*meta);
        *meta = nullptr;
      }
    };
    ArrayMetadata(Context &ctx, const std::string &uri) : ArrayMetadata(ctx) {
      _init(uri);
    }
    ArrayMetadata(const ArrayMetadata&) = default;
    ArrayMetadata(ArrayMetadata&& o) = default;
    ArrayMetadata &operator=(const ArrayMetadata&) = default;
    ArrayMetadata &operator=(ArrayMetadata &&o) = default;

    void load(const std::string &uri) {
      _init(uri);
    }

    ArrayMetadata &create(const std::string &uri);

    std::string to_str() const;

    tiledb_array_type_t type() const;

    uint64_t capacity() const {
      auto &ctx = _ctx.get();
      uint64_t capacity;
      ctx.handle_error(tiledb_array_metadata_get_capacity(ctx, _meta.get(), &capacity));
      return capacity;
    }

    tiledb_layout_t tile_layout() const;

    tiledb_layout_t cell_layout() const;

    Compressor coords_compressor() const;

    std::string name() const;

    Domain domain() const;

    ArrayMetadata &set_domain(const Domain &domain);

    ArrayMetadata &add_attribute(const Attribute &attr);

    void check() const;

    const std::unordered_map<std::string, Attribute> attributes() const;

    bool good() const {
      return _meta == nullptr;
    }
    std::shared_ptr<tiledb_array_metadata_t> ptr() const {
      return _meta;
    }


  private:
    friend class Array;

    void _init(tiledb_array_metadata_t* meta);
    void _init(const std::string &uri);

    struct _Deleter {
      _Deleter(Context& ctx) : _ctx(ctx) {}
      _Deleter(const _Deleter&) = default;
      void operator()(tiledb_array_metadata_t *p);
    private:
      std::reference_wrapper<Context> _ctx;
    };

    std::reference_wrapper<Context> _ctx;
    _Deleter _deleter;
    std::shared_ptr<tiledb_array_metadata_t> _meta;
  };
}

std::ostream &operator<<(std::ostream &os, const tdb::ArrayMetadata &meta);
tdb::ArrayMetadata &operator<<(tdb::ArrayMetadata &meta, const tdb::Domain &dim);
tdb::ArrayMetadata &operator<<(tdb::ArrayMetadata &meta, const tdb::Dimension &dim);
tdb::ArrayMetadata &operator<<(tdb::ArrayMetadata &meta, const tdb::Attribute &dim);
#endif //TILEDB_TDBPP_ARRAYMETA_H