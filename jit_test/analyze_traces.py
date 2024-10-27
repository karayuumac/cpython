import trace_debug
from test_trace_complex import process_points, Point, mixed_calculation

def analyze_traces():
    # トレース情報を収集して表示
    print("Analyzing process_points function:")
    traces = trace_debug.get_trace_info(process_points)

    for i, trace in enumerate(traces['traces']):
        print(f"\nTrace {i}:")
        print(f"Start offset: {trace['start_offset']}")
        print(f"Execution count: {trace.get('execution_count', 0)}")

        print("\nInstructions:")
        for inst in trace['instructions']:
            print(f"  {inst['opcode']} (arg={inst['oparg']})")
            print(f"    Stack types: {inst['stack_types']}")

        print("\nGuards:")
        for guard in trace['guards']:
            print(f"  Kind: {guard['kind']}, Stack index: {guard['stack_index']}")

def main():
    # まずホットトレースを生成
    points = [Point(1.0, 1.0), Point(2.0, 3.0)]
    for _ in range(1000):
        process_points(points, 100)

    # トレース分析を実行
    analyze_traces()

if __name__ == "__main__":
    main()