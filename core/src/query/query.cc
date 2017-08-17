/**
 * @file   query.h
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
 * This file implements class Query.
 */

#include "query.h"
#include "logger.h"
#include "utils.h"

/* ****************************** */
/*   CONSTRUCTORS & DESTRUCTORS   */
/* ****************************** */

namespace tiledb {

Query::Query() {
  subarray_ = nullptr;
}

Query::Query(
    Array* array,
    QueryMode mode,
    const void* subarray,
    const char** attributes,
    int attribute_num) {
  array_ = array;
  mode_ = mode;

  uint64_t subarray_size = 2 * array_->array_schema()->coords_size();
  subarray_ = malloc(subarray_size);
  if (subarray_ == nullptr) {
    LOG_STATUS(Status::QueryError("Memory allocation for subarray failed"));
    return;
  }

  if (subarray == nullptr)
    memcpy(subarray_, array_->array_schema()->domain(), subarray_size);
  else
    memcpy(subarray_, subarray, subarray_size);

  // Get attributes
  std::vector<std::string> attributes_vec;
  const ArraySchema* array_schema = array_->array_schema();
  if (attributes == nullptr) {  // Default: all attributes
    attributes_vec = array_schema->attributes();
    if (array_schema->dense() && mode != QueryMode::WRITE_UNSORTED)
      // Remove coordinates attribute for dense arrays,
      // unless in TILEDB_WRITE_UNSORTED mode
      attributes_vec.pop_back();
  } else {  // Custom attributes
    // Get attributes
    bool coords_found = false;
    bool sparse = !array_schema->dense();
    unsigned name_max_len = Configurator::name_max_len();
    for (int i = 0; i < attribute_num; ++i) {
      // Check attribute name length
      if (attributes[i] == nullptr || strlen(attributes[i]) > name_max_len) {
        LOG_STATUS(Status::QueryError("Invalid attribute name length"));
        // TODO: handle errors better
        return;
      }
      attributes_vec.emplace_back(attributes[i]);
      if (!strcmp(attributes[i], Configurator::coords()))
        coords_found = true;
    }

    // Sanity check on duplicates
    if (utils::has_duplicates(attributes_vec)) {
      LOG_STATUS(
          Status::QueryError("Cannot initialize array; Duplicate attributes"));
      // TODO: handle errors better
      return;
    }

    // For the case of sparse array, append coordinates if they do
    // not exist already
    if (sparse && !coords_found)
      attributes_vec.emplace_back(Configurator::coords());
  }

  // Set attribute ids
  Status st = array_schema->get_attribute_ids(attributes_vec, attribute_ids_);
  if (!st.ok())
    LOG_STATUS(st);
}

Query::~Query() {
  if (subarray_ != nullptr)
    free(subarray_);
}

/* ****************************** */
/*               API              */
/* ****************************** */

const std::vector<int>& Query::attribute_ids() const {
  return attribute_ids_;
}

Status Query::coords_buffer_i(int* coords_buffer_i) const {
  int buffer_i = 0;
  auto attribute_id_num = (int)attribute_ids_.size();
  int attribute_num = array_->array_schema()->attribute_num();
  for (int i = 0; i < attribute_id_num; ++i) {
    if (attribute_ids_[i] == attribute_num) {
      *coords_buffer_i = buffer_i;
      break;
    }
    if (!array_->array_schema()->var_size(attribute_ids_[i]))  // FIXED CELLS
      ++buffer_i;
    else  // VARIABLE-SIZED CELLS
      buffer_i += 2;
  }

  // Coordinates are missing
  if (*coords_buffer_i == -1)
    return LOG_STATUS(
        Status::ArrayError("Cannot find coordinates buffer index"));

  // Success
  return Status::Ok();
}

QueryMode Query::mode() const {
  return mode_;
}

Status Query::reset_subarray(const void* subarray) {
  uint64_t subarray_size = 2 * array_->array_schema()->coords_size();
  if (subarray_ == nullptr)
    subarray_ = malloc(subarray_size);

  if (subarray_ == nullptr)
    return LOG_STATUS(
        Status::QueryError("Memory allocation when resetting subarray"));

  if (subarray == nullptr)
    memcpy(subarray_, array_->array_schema()->domain(), subarray_size);
  else
    memcpy(subarray_, subarray, subarray_size);

  return Status::Ok();
}

const void* Query::subarray() const {
  return subarray_;
}

}  // namespace tiledb
