import numpy as np
import matplotlib.pyplot as plt
import ruptures as rpt
data_name="osm_cellids"
#osm_cellids_800M_uint64
#books_800M_uint64
#fb_200M_uint64
#wiki_ts_200M_uint64

raw = np.fromfile(f"/export/data/jspeng/learned_index/dataset/{data_name}_800M_uint64", dtype=np.uint64)

# 第一个元素是数据的长度，去掉它
data_length = raw[0]
data = raw[1:]
sorted_data = np.sort(data)
gaps = np.diff(sorted_data)
print(f"len(data): {len(data)}")

# plt.figure(figsize=(12, 6))
# plt.plot(gaps)
# plt.xlabel('Index', fontsize=12)
# plt.ylabel('Gap Value', fontsize=12)
# plt.title(f'{data_name}_Original_gap')
# plt.savefig(f'test/{data_name}_gap_original.png')  # 保存图像
# plt.close()

t=1
print(f"----------------第{t}次------------------")
# 对gaps进行均匀抽样，得出1000个点
sample_size = 1000
sample_indices = np.linspace(0, len(gaps) - 1, sample_size, dtype=int)
sampled_gaps = gaps[sample_indices]
# # 使用ruptures包查找GAP序列的断点，使用costrbf成本函数
cost = rpt.costs.CostRbf().fit(sampled_gaps)
model = rpt.Pelt(custom_cost=cost).fit(sampled_gaps)
breakpoints = model.predict(pen=10)
# 将断点映射回原始gaps数据的位置
breakpoints_original = [sample_indices[bp] for bp in breakpoints if bp < sample_size]

# 打印断点的位置
print("Detected breakpoints in sampled data:", breakpoints)
print("Mapped breakpoints in original data:", breakpoints_original)
new_breakpoints = [0] + breakpoints_original + [len(gaps)]
new_breakpoints = sorted(new_breakpoints)
print(f"第{t}次断点:{new_breakpoints}")
# 在原始gaps数据中绘制断点
# plt.figure(figsize=(12, 6))
# plt.plot(gaps)
# plt.xlabel('Index', fontsize=12)
# plt.ylabel('Gap Value', fontsize=12)
# plt.title(f'gap_breakpoints_{t}')
# for bp in breakpoints_original:  # 在图中标出断点
#     plt.axvline(x=bp, color='r', linestyle='--')
# plt.savefig(f'test/gap_breakpoints_{t}.png')  # 保存图像
# plt.close()  # 关闭图像以释放内存


while len(new_breakpoints)<10:
    t+=1
    print(f"----------------第{t}次------------------")
    all_empty = True  # 用于检查所有的 interval_breakpoints_original 是否为空
    for i in range(len(new_breakpoints)-1):
        start_idx = new_breakpoints[i]
        end_idx= new_breakpoints[i+1]
        print(f"第{i}个区间:{start_idx}~{end_idx}")
        interval_gaps = gaps[start_idx:end_idx]
        if len(interval_gaps) > sample_size:
            sample_indices = np.linspace(start_idx, end_idx - 1, sample_size, dtype=int)
        else:
            sample_indices = np.arange(start_idx, end_idx, dtype=int)
        sampled_gaps = gaps[sample_indices]

        if len(sampled_gaps) > 1:  # 需要至少两个点来寻找断点
            cost = rpt.costs.CostRbf().fit(sampled_gaps)
            model = rpt.Pelt(custom_cost=cost).fit(sampled_gaps)
            interval_breakpoints = model.predict(pen=10)
            # 将断点映射回原始gaps数据的位置
            interval_breakpoints_original = [sample_indices[bp] for bp in interval_breakpoints if bp < len(sample_indices)]
            print("Detected interval_breakpoints in sampled data:", interval_breakpoints)
            # 排除第一个和最后一个断点（它们是区间的边界）
            interval_breakpoints_original = [bp for bp in interval_breakpoints_original if bp != start_idx and bp != end_idx]
            print(f"Mapped interval_breakpoints_original:{interval_breakpoints_original}")

            if interval_breakpoints_original:
                all_empty = False  # 如果 interval_breakpoints_original 非空，更新 all_empty 标志
                new_breakpoints.extend(interval_breakpoints_original)
            # new_breakpoints.extend(interval_breakpoints_original)
    if all_empty:
        print("所有的 Mapped interval_breakpoints_original 都为空，算法停止")
        break

    # 去重并排序
    new_breakpoints = sorted(set(new_breakpoints))
    print(f"第{t}次取样的断点:{new_breakpoints}")
    # 在原始gaps数据中绘制所有断点
    # plt.figure(figsize=(12, 6))
    # plt.plot(gaps)
    # plt.xlabel('Index', fontsize=12)
    # plt.ylabel('Gap Value', fontsize=12)
    # plt.title(f'gap_breakpoints_{t}')
    # for bp in new_breakpoints:  # 在图中标出所有断点
    #     plt.axvline(x=bp, color='r', linestyle='--')
    # plt.savefig(f'test/gap_breakpoints_{t}.png')  # 保存图像
    # plt.close()  # 关闭图像以释放内存

# 在原始gaps数据中绘制所有断点
plt.figure(figsize=(6, 3))
plt.plot(gaps)
# plt.xscale('log')
plt.xlabel('Index', fontsize=12)
plt.ylabel('Gap Value', fontsize=12)
plt.title(f'{data_name}_gap_breakpoints')
for bp in new_breakpoints:  # 在图中标出所有断点
    plt.axvline(x=bp, color='r', linestyle='--')
plt.savefig(f'test/{data_name}_gap_breakpoints.png')  # 保存图像
plt.close()  # 关闭图像以释放内存

