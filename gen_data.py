import random

# 配置
FILENAME = "test.csv"
ORDER_COUNT = 10  # 100万单
USERS = range(1000, 2000) # 模拟1000个用户

print(f"Generating {ORDER_COUNT} orders...")

with open(FILENAME, "w") as f:
    for i in range(ORDER_COUNT):
        # 1. 用户ID
        user_id = random.choice(USERS)
        
        # 2. 方向 (买/卖)
        side = random.choice(["B", "A"])
        
        # 3. 价格 (高斯分布，模拟真实盘口集中在100附近)
        # 均值100，标准差5，生成 90~110 之间的密集价格
        price = int(random.gauss(100, 5))
        if price <= 0: price = 1
        
        # 4. 数量 (1 ~ 100)
        amount = random.randint(1, 100)
        
        # 写入文件
        f.write(f"{user_id}, {side}, {price}, {amount}\n")

print(f"Done! Saved to {FILENAME}")