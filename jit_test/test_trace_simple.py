def calculate_sum(n):
    total = 0
    i = 0
    while i < n:
        total += i
        i += 1
    return total

def main():
    # 実行回数を増やしてホットトレースを発生させる
    for _ in range(100):
        result = calculate_sum(1000)
    print(f"Result: {result}")

if __name__ == "__main__":
    main()