from typing import List, Union
import time

class Point:
    def __init__(self, x: float, y: float):
        self.x = x
        self.y = y

    def move(self, dx: float, dy: float):
        self.x += dx
        self.y += dy
        return self

def process_points(points: List[Point], iterations: int) -> List[float]:
    results = []

    for i in range(iterations):
        total_distance = 0.0

        for p in points:
            # ポイントを動かす
            p.move(0.1 * i, 0.2 * i)

            # 原点からの距離を計算
            distance = (p.x * p.x + p.y * p.y) ** 0.5
            total_distance += distance

        results.append(total_distance)

    return results

def mixed_calculation(values: List[Union[int, float]], count: int) -> float:
    """型の混在するリストに対する計算"""
    result = 0.0

    for _ in range(count):
        for v in values:
            if isinstance(v, int):
                result += v * 2
            else:
                result += v * 1.5

    return result

def main():
    # テストケース1: Point操作のトレース
    points = [Point(1.0, 1.0), Point(2.0, 3.0), Point(4.0, 5.0)]

    # ホットトレースを発生させるため複数回実行
    start_time = time.time()
    for _ in range(100):
        results = process_points(points, 1000)
    print(f"Final distance: {results[-1]}")
    print(f"Time taken: {time.time() - start_time:.3f}s")

    # テストケース2: 混合型計算のトレース
    values = [1, 2.5, 3, 4.5, 5, 6.5]

    start_time = time.time()
    for _ in range(100):
        result = mixed_calculation(values, 1000)
    print(f"Mixed calculation result: {result}")
    print(f"Time taken: {time.time() - start_time:.3f}s")

if __name__ == "__main__":
    main()