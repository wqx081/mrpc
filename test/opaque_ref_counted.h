#ifndef BASE_TEST_OPAQUE_REF_COUNTED_H_
#define BASE_TEST_OPAQUE_REF_COUNTED_H_

#include "base/ref_counted.h"

namespace mrpc {

// OpaqueRefCounted is a test class for scoped_refptr to ensure it still works
// when the pointed-to type is opaque (i.e., incomplete).
class OpaqueRefCounted;

// Test functions that return and accept scoped_refptr<OpaqueRefCounted> values.
scoped_refptr<OpaqueRefCounted> MakeOpaqueRefCounted();
void TestOpaqueRefCounted(scoped_refptr<OpaqueRefCounted> p);

}  // namespace base

extern template class scoped_refptr<mrpc::OpaqueRefCounted>;

#endif  // BASE_TEST_OPAQUE_REF_COUNTED_H_
