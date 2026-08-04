import random

# Synthetic workload configuration.
FILENAME = "test.csv"
ORDER_COUNT = 10000000
USERS = range(1000, 2000)

print(f"Generating {ORDER_COUNT} orders...")

with open(FILENAME, "w") as f:
    for i in range(ORDER_COUNT):
        user_id = random.choice(USERS)

        side = random.choice(["B", "A"])

        # Concentrate prices around 100 to create a dense synthetic distribution.
        price = int(random.gauss(100, 5))
        if price <= 0: price = 1

        amount = random.randint(1, 100)

        f.write(f"{user_id}, {side}, {price}, {amount}\n")

print(f"Done! Saved to {FILENAME}")
