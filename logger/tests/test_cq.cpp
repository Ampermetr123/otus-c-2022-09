#include <cqueue.h>
#include <gtest/gtest.h>
#include <iostream>

class CQueueTest : public ::testing::Test {
protected:
  void SetUp() override {
    cq = NULL;
    cq2 = NULL;
  }

  void TearDown() override {
    if (cq)
      cq_free(cq);
    if (cq2)
      cq_free(cq2);
  }

  CQueue *cq;
  CQueue *cq2;
};


TEST_F(CQueueTest, is_empty_initially) {
  cq = cq_new(512, true);
  ASSERT_TRUE(cq != NULL);
  ASSERT_EQ(cq_length(cq), 0);
  char *ostr;
  unsigned osz;
  auto ret = cq_top(cq, &ostr, &osz);
  ASSERT_EQ(ret, cq_res_queue_empty);
}


TEST_F(CQueueTest, insert_with_head_overlap) {
  cq = cq_new(48, true);
  ASSERT_TRUE(cq != NULL);

  int ret = cq_push(cq, "str1", 5);
  ASSERT_EQ(ret, cq_res_ok);
  ASSERT_EQ(cq_length(cq), 1);

  ret = cq_push(cq, "str2", 5);
  ASSERT_EQ(ret, cq_res_ok);
  ASSERT_EQ(cq_length(cq), 2);

  ret = cq_push(cq, "str3", 5);
  ASSERT_EQ(ret, cq_res_lost_head);
  ASSERT_EQ(cq_length(cq), 2);

  char *ostr;
  unsigned osz;
  ret = cq_top(cq, &ostr, &osz);
  ASSERT_EQ(ret, cq_res_ok);
  ASSERT_EQ(5, osz);
  ASSERT_STREQ("str2", ostr);
  ASSERT_EQ(cq_length(cq), 2);

  cq_pop(cq);
  ASSERT_EQ(cq_length(cq), 1);
  ret = cq_top(cq, &ostr, &osz);
  ASSERT_EQ(ret, cq_res_ok);
  ASSERT_EQ(5, osz);
  ASSERT_STREQ("str3", ostr);

  cq_pop(cq);
  ASSERT_EQ(cq_length(cq), 0);
  ret = cq_top(cq, &ostr, &osz);
  ASSERT_EQ(ret, cq_res_queue_empty);
}


TEST_F(CQueueTest, insert_with_multy_loop) {
  static const int max_size = 1000;
  char *ostr;
  unsigned osz;
  cq = cq_new(max_size, true);
  ASSERT_TRUE(cq != NULL);
  union FillStruct {
    int val;
    char data[max_size];
  } st;
  int sz = max_size / 2;
  for (int i = 1; i < 50; i++) {
    st.val = i;
    auto push_ret = cq_push(cq, (char *)&st, sz / i);
    auto sz_new = cq_length(cq);
    auto top_ret = cq_top(cq, &ostr, &osz);
    ASSERT_EQ(top_ret, cq_res_ok) << "on i=" << i << " after push with ret=" << push_ret;
  }
  int j = 0;
  int prev = 0;
  auto len = cq_length(cq);
  auto ret = cq_top(cq, &ostr, &osz);
  ASSERT_EQ(ret, cq_res_ok);

  while (cq_top(cq, &ostr, &osz) == cq_res_ok) {
    j++;
    int current = ((FillStruct *)ostr)->val;
    ASSERT_TRUE(current > prev) << "cur=" << current << "; prev=" << prev;
    prev = current;
    cq_pop(cq);
  }
}


TEST_F(CQueueTest, push_from_iter) {
  char *ostr;
  unsigned osz;
  int ret;
  cq = cq_new(128, true);
  ret = cq_push(cq, "s1", 3);
  ASSERT_EQ(ret, cq_res_ok);
  ret = cq_push(cq, "s2", 3);
  ASSERT_EQ(ret, cq_res_ok);
  ret = cq_push(cq, "s3", 3);
  ASSERT_EQ(ret, cq_res_ok);
  ret = cq_push(cq, "s4", 3);
  ASSERT_EQ(ret, cq_res_ok);

  cq2 = cq_new(128, true);

  cq_iterator_t it = cq_begin(cq);

  cq_push_from_iter(cq2, cq, it);
  ASSERT_EQ(ret, cq_res_ok);
  it = cq_next(cq, it);
  it = cq_next(cq, it);
  cq_push_from_iter(cq2, cq, it);
  ASSERT_EQ(ret, cq_res_ok);

  ret = cq_top(cq2, &ostr, &osz);
  ASSERT_EQ(ret, cq_res_ok);
  ASSERT_EQ(3, osz);
  ASSERT_STREQ("s1", ostr);

  cq_pop(cq2);

  ret = cq_top(cq2, &ostr, &osz);
  ASSERT_EQ(ret, cq_res_ok);
  ASSERT_EQ(3, osz);
  ASSERT_STREQ("s3", ostr);
  ASSERT_EQ(1, cq_length(cq2));
}