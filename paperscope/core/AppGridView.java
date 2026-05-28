package com.google.vr.cardboard.paperscope.demo;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.GridView;

/**
 * This class holds the {@link CardView} elements that are displayed in the {@link MainActivity}.
 *
 * <p>Customized {@link GridView} that correctly sets its height to {@link
 * android.view.ViewGroup.LayoutParams#WRAP_CONTENT}.
 *
 * @see <a href="go/gridview-wrap-content">Related discussion on StackOverflow</a>.
 */
public class AppGridView extends GridView {
  public AppGridView(Context context) {
    super(context);
  }

  public AppGridView(Context context, AttributeSet attrs) {
    super(context, attrs);
  }

  public AppGridView(Context context, AttributeSet attrs, int defStyle) {
    super(context, attrs, defStyle);
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    int heightSpec;

    if (getLayoutParams().height == LayoutParams.WRAP_CONTENT) {
      // The two leftmost bits in the height measure spec have
      // a special meaning, hence we can't use them to describe height.
      heightSpec = MeasureSpec.makeMeasureSpec(Integer.MAX_VALUE >> 2, MeasureSpec.AT_MOST);
    } else {
      // Any other height should be respected as is.
      heightSpec = heightMeasureSpec;
    }

    super.onMeasure(widthMeasureSpec, heightSpec);
  }
}
