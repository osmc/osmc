<div class="sidebar column fourth">
    <?php if ( is_active_sidebar( 'sidebar1' ) ) : ?>

        <?php dynamic_sidebar( 'sidebar1' ); ?>

    <?php else : ?>

        <?php // This content shows up if there are no widgets defined in the backend. ?>

        <div class="alert alert-help">
            <p><?php _e( 'Please activate some Widgets.', 'bonestheme' );  ?></p>
        </div>

    <?php endif; ?>
</div>